#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Cash Node developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test the per-peer traffic rate-limiting feature.
"""

import time

from test_framework.messages import ser_string
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


timeout = 5
one_mb = 1000 * 1000
just_under_1mb = one_mb - 1000


class msg_bigdummy:
    """A custom message type to send a large, single payload for traffic tests."""
    command = b"bigdummy"
    msgtype = command

    def __init__(self, payload=b''):
        self.payload = payload

    def serialize(self):
        return ser_string(self.payload)

    def __str__(self):
        return f"msg_bigdummy(payload_size={len(self.payload)})"


class PeerRateLimitTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.log.info("Starting tests for per-peer traffic rate-limiting")

        # Kick node out of IBD because per-peer rate limiting is disabled if node is in IBD
        self.nodes[0].generatetoaddress(1, self.nodes[0].get_deterministic_priv_key().address)

        self.mock_time = int(time.time())
        self.nodes[0].setmocktime(self.mock_time)

        self.test_basic_ban()
        self.test_window_reset()
        self.test_ban_persistence()
        self.test_zero_ban_time()
        self.test_no_rules_no_ban()
        self.test_multiple_rules_worst_ban()
        self.test_per_peer_isolation()

    def send_dummy_message(self, peer, size):
        peer.send_message(msg_bigdummy(payload=b'\x00' * size))
        time.sleep(0.1) # Give the network thread time to respond

    def advance_mocktime(self, seconds):
        """Helper to advance mock time."""
        self.mock_time += seconds
        self.nodes[0].setmocktime(self.mock_time)

    def test_basic_ban(self):
        # Use a small limit for quick testing (1MB over 1 minute, ban for 2 minutes)
        self.restart_node(0, extra_args=[
            "-peerratelimit=1MB/1m:2m",
            f"-mocktime={self.mock_time}"
        ])
        peer = self.nodes[0].add_p2p_connection(P2PInterface())

        # Send 1MB to trigger the rate limit
        self.send_dummy_message(peer, one_mb)

        # Wait for the node to process the message and disconnect the peer
        peer.wait_for_disconnect(timeout)

        # Verify the peer was banned
        banned = self.nodes[0].listbanned()
        assert_equal(len(banned), 1)
        assert peer.dstaddr in banned[0]['address']

        # Verify the ban duration is correct.
        ban_duration = banned[0]['banned_until'] - banned[0]['ban_created']
        assert_equal(ban_duration, 120)

    def test_window_reset(self):
        self.restart_node(0, extra_args=[
            "-peerratelimit=1MB/1m:2m",
            f"-mocktime={self.mock_time}"
        ])
        self.nodes[0].clearbanned()
        peer = self.nodes[0].add_p2p_connection(P2PInterface())

        # Send traffic just under the limit
        self.send_dummy_message(peer, just_under_1mb)
        assert peer.is_connected, "Peer should not be disconnected after sending less than the limit"

        # Advance time past the 1-minute window
        self.advance_mocktime(60)

        # Send more traffic. If the window reset, this should not trigger a ban
        self.send_dummy_message(peer, just_under_1mb)

        assert peer.is_connected, "Peer should not be disconnected after sending more than the limit after window reset"

    def test_ban_persistence(self):
        # Test that bans persist after a node restart
        self.restart_node(0, extra_args=[
            "-peerratelimit=1MB/1m:2m",
            f"-mocktime={self.mock_time}"
        ])
        self.nodes[0].clearbanned()
        peer = self.nodes[0].add_p2p_connection(P2PInterface())

        # Send 1MB to trigger ban
        self.send_dummy_message(peer, one_mb)

        peer.wait_for_disconnect(timeout)
        assert_equal(len(self.nodes[0].listbanned()), 1)
        banned_peer_address = self.nodes[0].listbanned()[0]['address']

        # Advance time slightly before restart
        self.advance_mocktime(15)
        self.restart_node(0, extra_args=[
            "-peerratelimit=1MB/1m:2m",
            f"-mocktime={self.mock_time}"
        ])

        # Verify the ban is still in place after restart
        banned = self.nodes[0].listbanned()
        assert_equal(len(banned), 1)
        assert banned_peer_address in banned[0]['address']

    def test_zero_ban_time(self):
        # Test rule with zero ban time (disconnect only)
        self.restart_node(0, extra_args=[
            "-peerratelimit=1MB/1m:0m",
            f"-mocktime={self.mock_time}"
        ])
        self.nodes[0].clearbanned()
        peer = self.nodes[0].add_p2p_connection(P2PInterface())

        # Send 1MB to trigger ban
        self.send_dummy_message(peer, one_mb)

        peer.wait_for_disconnect(timeout)

        # Assert that the peer was disconnected but NOT banned
        banned = self.nodes[0].listbanned()
        assert_equal(len(banned), 0)

    def test_no_rules_no_ban(self):
        # Test that no action is taken when no rules are set
        self.restart_node(0, extra_args=[
            f"-mocktime={self.mock_time}"
        ])
        self.nodes[0].clearbanned()
        peer = self.nodes[0].add_p2p_connection(P2PInterface())

        # Send 1MB to attempt to trigger ban
        self.send_dummy_message(peer, one_mb)

        assert peer.is_connected, "Peer should not be disconnected when no rules are set"
        peer.peer_disconnect()

    def test_multiple_rules_worst_ban(self):
        # Test the 'worst violation' logic.

        rules = [
            "-peerratelimit=1MB/1m:1m",
            "-peerratelimit=2MB/5m:5m",
        ]

        # --- Scenario 1: Weaker Rule ---
        self.restart_node(0, extra_args=rules + [f"-mocktime={self.mock_time}"])
        peer1 = self.nodes[0].add_p2p_connection(P2PInterface())
        self.send_dummy_message(peer1, one_mb)
        peer1.wait_for_disconnect(timeout)
        banned = self.nodes[0].listbanned()
        assert_equal(len(banned), 1)
        assert_equal(banned[0]['banned_until'] - banned[0]['ban_created'], 60)

        # --- Scenario 2: Stronger Rule ---
        self.nodes[0].clearbanned()
        assert_equal(len(self.nodes[0].listbanned()), 0)
        peer2 = self.nodes[0].add_p2p_connection(P2PInterface())
        self.advance_mocktime(60)
        self.send_dummy_message(peer2, just_under_1mb)
        self.advance_mocktime(60)
        self.send_dummy_message(peer2, just_under_1mb)
        self.advance_mocktime(60)
        self.send_dummy_message(peer2, just_under_1mb)
        peer2.wait_for_disconnect(timeout)
        banned = self.nodes[0].listbanned()
        assert_equal(len(banned), 1)
        assert_equal(banned[0]['banned_until'] - banned[0]['ban_created'], 300)

    def test_per_peer_isolation(self):
        # Test that a misbehaving peer does not affect other peers
        self.restart_node(0, extra_args=[
            "-peerratelimit=1MB/1m:1m",
            f"-mocktime={self.mock_time}"
        ])
        self.nodes[0].clearbanned()

        peer_bad = self.nodes[0].add_p2p_connection(P2PInterface())
        peer_good = self.nodes[0].add_p2p_connection(P2PInterface())

        # Send 1MB to attempt to trigger ban
        self.send_dummy_message(peer_bad, one_mb)
        peer_bad.wait_for_disconnect(timeout)

        # Check that the bad peer is banned
        assert_equal(len(self.nodes[0].listbanned()), 1)

        # Check that the good peer is still connected
        assert peer_good.is_connected, "Good peer was disconnected unexpectedly"


if __name__ == '__main__':
    PeerRateLimitTest().main()
