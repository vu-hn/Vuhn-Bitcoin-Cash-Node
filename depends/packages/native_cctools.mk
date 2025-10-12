package=native_cctools
$(package)_version=55943b0c68c0eaf8b8ad2f51f63738bbc7b0c86b
$(package)_download_path=https://github.com/tpoechtrager/cctools-port/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=1f9da0f0d11f5b7275a879b4bf02ef5f87e006acff96c74fc00ee2b2b44e25d6
$(package)_build_subdir=cctools
$(package)_patches=ld64_disable_threading.patch

$(package)_libtapi_version=aed9334283e3e290bba622ee980bde2322e4d516
$(package)_libtapi_download_path=https://github.com/tpoechtrager/apple-libtapi/archive
$(package)_libtapi_download_file=$($(package)_libtapi_version).tar.gz
$(package)_libtapi_file_name=$($(package)_libtapi_version).tar.gz
$(package)_libtapi_sha256_hash=f244f7b3d8de3277204dece255b531f976bee80dad3247e9f36b30052b91c959

$(package)_libdispatch_version=323b9b4e0ca05d6c56a0c2f2d7d8d47363e612b7
$(package)_libdispatch_download_path=https://github.com/tpoechtrager/apple-libdispatch/archive
$(package)_libdispatch_download_file=$($(package)_libdispatch_version).tar.gz
$(package)_libdispatch_file_name=$($(package)_libdispatch_version).tar.gz
$(package)_libdispatch_sha256_hash=ac65cff5ed89903d02e2722162373ecac3b782846ab133dcc2306bf6c6872482

$(package)_extra_sources=$($(package)_libtapi_file_name)
$(package)_extra_sources+=$($(package)_libdispatch_file_name)

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_libtapi_download_path),$($(package)_libtapi_download_file),$($(package)_libtapi_file_name),$($(package)_libtapi_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_libdispatch_download_path),$($(package)_libdispatch_download_file),$($(package)_libdispatch_file_name),$($(package)_libdispatch_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_libtapi_sha256_hash)  $($(package)_source_dir)/$($(package)_libtapi_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_libdispatch_sha256_hash)  $($(package)_source_dir)/$($(package)_libdispatch_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir -p libtapi && \
  tar --no-same-owner --strip-components=1 -C libtapi -xf $($(package)_source_dir)/$($(package)_libtapi_file_name) && \
  mkdir -p libdispatch && \
  tar --no-same-owner --strip-components=1 -C libdispatch -xf $($(package)_source_dir)/$($(package)_libdispatch_file_name) && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source)
endef

define $(package)_set_vars
  $(package)_config_opts=--target=$(host) --with-libtapi=$($(package)_extract_dir) --with-libdispatch=$($(package)_extract_dir) --with-libblocksruntime=$($(package)_extract_dir)
  $(package)_ldflags+=-Wl,-rpath=\\$$$$$$$$\$$$$$$$$ORIGIN/../lib
  $(package)_config_opts+=--enable-lto-support
  $(package)_cc=clang
  $(package)_cxx=clang++
endef

define $(package)_preprocess_cmds
  CC=$($(package)_cc) CXX=$($(package)_cxx) INSTALLPREFIX=$($(package)_extract_dir) ./libtapi/build.sh && \
  CC=$($(package)_cc) CXX=$($(package)_cxx) INSTALLPREFIX=$($(package)_extract_dir) ./libtapi/install.sh && \
  patch -p1 < $($(package)_patch_dir)/ld64_disable_threading.patch && \
  mkdir -p libdispatch/build && \
  cmake -GNinja -DCMAKE_C_COMPILER=$($(package)_cc) -DCMAKE_CXX_COMPILER=$($(package)_cxx) -DCMAKE_INSTALL_PREFIX=$($(package)_extract_dir) -S libdispatch -B libdispatch/build && \
  cmake --build libdispatch/build && \
  cmake --install libdispatch/build
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) -j$(JOBS)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install && \
  mkdir -p $($(package)_staging_prefix_dir)/lib/ && \
  cd $($(package)_extract_dir) && \
  cp -vf lib/libtapi.so.17 lib/libBlocksRuntime.so lib/libdispatch.so $($(package)_staging_prefix_dir)/lib/
endef
