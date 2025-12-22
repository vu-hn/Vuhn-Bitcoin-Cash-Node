// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <peerratelimiter.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <optional>

BOOST_FIXTURE_TEST_SUITE(peerratelimiter_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(parse_peerratelimit_valid) {
    // Standard rule with all components
    PeerRateLimitRule rule1("100MB/1h:1d");
    BOOST_CHECK(rule1.IsValid());
    BOOST_CHECK_EQUAL(rule1.GetLimitMB(), 100);
    BOOST_CHECK_EQUAL(rule1.GetWindowMinutes(), 60);
    BOOST_CHECK_EQUAL(rule1.GetBanMinutes(), 1440); // 1 day = 1440 minutes

    // Different units (KB, GB, TB, m, w)
    PeerRateLimitRule rule2("2GB/30m:2w");
    BOOST_CHECK(rule2.IsValid());
    BOOST_CHECK_EQUAL(rule2.GetLimitMB(), 2000); // 2 GB = 2000 MB
    BOOST_CHECK_EQUAL(rule2.GetWindowMinutes(), 30);
    BOOST_CHECK_EQUAL(rule2.GetBanMinutes(), 20160); // 2 weeks = 20160 minutes

    PeerRateLimitRule rule3("3TB/2d:48h");
    BOOST_CHECK(rule3.IsValid());
    BOOST_CHECK_EQUAL(rule3.GetLimitMB(), 3000000); // 3 TB = 3000000 MB
    BOOST_CHECK_EQUAL(rule3.GetWindowMinutes(), 2880); // 2 days = 2880 minutes
    BOOST_CHECK_EQUAL(rule3.GetBanMinutes(), 2880); // 48 hours = 2880 minutes

    // Zero ban duration (means just kick without a ban)
    PeerRateLimitRule rule4("50MB/10m:0");
    BOOST_CHECK(rule4.IsValid());
    BOOST_CHECK_EQUAL(rule4.GetLimitMB(), 50);
    BOOST_CHECK_EQUAL(rule4.GetWindowMinutes(), 10);
    BOOST_CHECK_EQUAL(rule4.GetBanMinutes(), 0);

    // Whitespace
    PeerRateLimitRule rule5("  20MB / 5m : 1h  ");
    BOOST_CHECK(rule5.IsValid());
    BOOST_CHECK_EQUAL(rule5.GetLimitMB(), 20);
    BOOST_CHECK_EQUAL(rule5.GetWindowMinutes(), 5);
    BOOST_CHECK_EQUAL(rule5.GetBanMinutes(), 60);
}

BOOST_AUTO_TEST_CASE(parse_peerratelimit_invalid) {
    // Malformed strings
    BOOST_CHECK(!PeerRateLimitRule("").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("/").IsValid());
    BOOST_CHECK(!PeerRateLimitRule(":").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("/:").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB/").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB/1h").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB/1h:").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("/1h").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("/1h:").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("/1h:1h").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1h:1h").IsValid());
    BOOST_CHECK(!PeerRateLimitRule(":1h").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("abc/def:ghi").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB/1h:").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB/1h,1h").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB:1h/1h").IsValid());

    // Invalid units or values
    BOOST_CHECK(!PeerRateLimitRule("1MB/1y:1d").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1EB/1h:1d").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("0MB/1h:1d").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB/0m:1d").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("-1MB/1h:1d").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB/-1h:1d").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB/1h:-1d").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB/1y:1d").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1h/1h:1d").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB/1MB:1d").IsValid());
    BOOST_CHECK(!PeerRateLimitRule("1MB/1h:1MB").IsValid());
}

BOOST_AUTO_TEST_CASE(peerratelimitrule_get_description)
{
    // Test GetDescription
    PeerRateLimitRule rule1("1MB/1m:1m");
    BOOST_CHECK_EQUAL(rule1.GetDescription(), "1 MB transferred within 1 minute -> ban for 1 minute");

    PeerRateLimitRule rule2("999MB/120m:1h");
    BOOST_CHECK_EQUAL(rule2.GetDescription(), "999 MB transferred within 2 hours -> ban for 1 hour");

    PeerRateLimitRule rule3("1000/1d:2w");
    BOOST_CHECK_EQUAL(rule3.GetDescription(), "1 GB transferred within 1 day -> ban for 2 weeks");

    PeerRateLimitRule rule4("25TB/1m:10000w");
    BOOST_CHECK_EQUAL(rule4.GetDescription(), "25 TB transferred within 1 minute -> ban for 10000 weeks");
}

BOOST_AUTO_TEST_CASE(sliding_window_accumulation)
{
    // Test that traffic within the window accumulates correctly
    // Use a 60-second window with 1-second buckets for easy testing
    SlidingWindowCounter counter(60, 1);
    int64_t now = 1000; // Arbitrary start time

    // Add 100MB at T=1000s
    BOOST_CHECK_EQUAL(counter.Add(100, now), 100);

    // Add 50MB at T=1010s. Total should be 150MB
    now += 10;
    BOOST_CHECK_EQUAL(counter.Add(50, now), 150);
}

BOOST_AUTO_TEST_CASE(sliding_window_full_expiration)
{
    // Test that traffic expires completely after the window passes
    SlidingWindowCounter counter(60, 1);
    int64_t now = 1000;

    BOOST_CHECK_EQUAL(counter.Add(100, now), 100);

    // Advance to just before the end of the window (59 seconds later) without adding any more data
    now += 59;
    BOOST_CHECK_EQUAL(counter.Add(0, now), 100);

    // Advance to the exact window's end (60 seconds later) without adding data. The count should drop to zero.
    // The bucket from T=1000 is now expired at T=1060.
    now += 1;
    BOOST_CHECK_EQUAL(counter.Add(0, now), 0);
}

BOOST_AUTO_TEST_CASE(sliding_window_partial_expiration)
{
    // This is the most important test for a *sliding* window
    // It verifies that traffic expires incrementally
    SlidingWindowCounter counter(60, 1);
    int64_t now = 1000;

    // Add 100MB at T=1000s
    counter.Add(100, now);

    // Add 200MB at T=1030s. Total is 300MB
    now += 30;
    BOOST_CHECK_EQUAL(counter.Add(200, now), 300);

    // Advance time to T=1070s
    // The first 100MB (from T=1000s) should have expired because 1070 - 1000 > 60
    // The second 200MB (from T=1030s) should still be valid because 1070 - 1030 < 60
    now += 40;
    BOOST_CHECK_EQUAL(counter.Add(0, now), 200);

    // Advance time to T=1100s
    // Now the second 200MB (from T=1030s) should also have expired because 1100 - 1030 > 60
    now += 30;
    BOOST_CHECK_EQUAL(counter.Add(0, now), 0);
}

BOOST_AUTO_TEST_CASE(sliding_window_advance_time_backwards)
{
    // Test that going back in time has no effect
    SlidingWindowCounter counter(60, 1);
    int64_t now = 1000;

    now += 30;
    BOOST_CHECK_EQUAL(counter.Add(100, now), 100);

    // Advancing to a time before the last update should do nothing
    int64_t time_in_past = now - 10;
    BOOST_CHECK_EQUAL(counter.Add(0, time_in_past), 100);

    // The count should remain unchanged when we query again at the current time
    BOOST_CHECK_EQUAL(counter.Add(0, now), 100);

    // Add more data to confirm accumulation still works
    now += 1;
    BOOST_CHECK_EQUAL(counter.Add(50, now), 150);
}

BOOST_AUTO_TEST_CASE(sliding_window_large_time_jump)
{
    // 60-second window with 1-second buckets
    SlidingWindowCounter counter(60, 1);
    int64_t now = 1000;

    BOOST_CHECK_EQUAL(counter.Add(100, now), 100);

    // Jump forward in time by 10 window lengths (600 seconds)
    now += 600;

    // All data should have expired, and the usage should be zero
    BOOST_CHECK_EQUAL(counter.Add(0, now), 0);

    // Add more data to ensure the counter still works after the jump
    counter.Add(50, now);
    BOOST_CHECK_EQUAL(counter.Add(0, now), 50);
}

BOOST_AUTO_TEST_CASE(client_usage_tracker_no_violation) {
    int64_t now = 1000;
    std::vector<PeerRateLimitRule> rules = {
        PeerRateLimitRule("100MB/1m:5m")
    };
    ClientUsageTracker tracker(rules);

    // Record traffic under the limit (99 MB in bytes)
    auto violation = tracker.RecordTransfer(99 * 1000 * 1000, now);

    BOOST_CHECK(!violation.has_value());
}

BOOST_AUTO_TEST_CASE(client_usage_tracker_single_violation) {
    int64_t now = 1000;
    PeerRateLimitRule rule("50MB/1m:5m");
    ClientUsageTracker tracker({rule});

    // Record traffic exactly at the limit
    auto violation = tracker.RecordTransfer(50 * 1000 * 1000, now);

    BOOST_CHECK(violation.has_value());
    // Check if the returned rule is the one that was violated
    BOOST_CHECK_EQUAL(violation->GetName(), rule.GetName());
    BOOST_CHECK_EQUAL(violation->GetLimitMB(), 50);
}

BOOST_AUTO_TEST_CASE(client_usage_tracker_multiple_rules_worst_violation) {
    int64_t now = 1000;
    PeerRateLimitRule ruleA("100MB/1m:5m");  // Shorter ban
    PeerRateLimitRule ruleB("200MB/1m:10m"); // Longer ban
    PeerRateLimitRule ruleC("300MB/1m:2m");  // Shortest ban

    ClientUsageTracker tracker({ruleA, ruleB, ruleC});

    // 1. Violate only Rule A
    auto violation1 = tracker.RecordTransfer(150 * 1000 * 1000, now);
    BOOST_CHECK(violation1.has_value());
    BOOST_CHECK_EQUAL(violation1->GetName(), ruleA.GetName());

    // 2. Violate Rule A and B. Rule B should be chosen as it has the longest ban
    auto violation2 = tracker.RecordTransfer(100 * 1000 * 1000, now); // Total is now 250MB
    BOOST_CHECK(violation2.has_value());
    BOOST_CHECK_EQUAL(violation2->GetName(), ruleB.GetName());

    // 3. Violate all rules. Rule B should still be the "worst"
    auto violation3 = tracker.RecordTransfer(100 * 1000 * 1000, now); // Total is now 350MB
    BOOST_CHECK(violation3.has_value());
    BOOST_CHECK_EQUAL(violation3->GetName(), ruleB.GetName());
}

BOOST_AUTO_TEST_CASE(client_usage_tracker_setrules_clears_old_state) {
    std::optional<ClientUsageTracker> tracker;
    int64_t now = 1000;
    PeerRateLimitRule ruleA("10MB/1m:5m");
    tracker.emplace({ruleA});

    // Violate the first rule
    auto violation1 = tracker->RecordTransfer(11 * 1000 * 1000, now);
    BOOST_CHECK(violation1.has_value());
    BOOST_CHECK_EQUAL(violation1->GetName(), ruleA.GetName());

    // Now, set new rules. This should clear the state of the old counters
    PeerRateLimitRule ruleB("100MB/1m:10m");
    tracker.emplace({ruleB});

    // Check for violations again. There should be none, as the new counter starts at 0
    auto violation2 = tracker->RecordTransfer(0, now);
    BOOST_CHECK(!violation2.has_value());

    // Now violate the new rule
    auto violation3 = tracker->RecordTransfer(101 * 1000 * 1000, now);
    BOOST_CHECK(violation3.has_value());
    BOOST_CHECK_EQUAL(violation3->GetName(), ruleB.GetName());
}

BOOST_AUTO_TEST_SUITE_END()
