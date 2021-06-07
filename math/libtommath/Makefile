############################################################################
# apps/math/libtommath/Makefile
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

CFLAGS += -Wno-format

CSRCS = bn_cutoffs.c bn_deprecated.c bn_mp_2expt.c bn_mp_abs.c bn_mp_add.c bn_mp_add_d.c bn_mp_addmod.c \
bn_mp_and.c bn_mp_clamp.c bn_mp_clear.c bn_mp_clear_multi.c bn_mp_cmp.c bn_mp_cmp_d.c bn_mp_cmp_mag.c \
bn_mp_cnt_lsb.c bn_mp_complement.c bn_mp_copy.c bn_mp_count_bits.c bn_mp_decr.c bn_mp_div.c bn_mp_div_2.c \
bn_mp_div_2d.c bn_mp_div_3.c bn_mp_div_d.c bn_mp_dr_is_modulus.c bn_mp_dr_reduce.c bn_mp_dr_setup.c \
bn_mp_error_to_string.c bn_mp_exch.c bn_mp_expt_u32.c bn_mp_exptmod.c bn_mp_exteuclid.c bn_mp_fread.c \
bn_mp_from_sbin.c bn_mp_from_ubin.c bn_mp_fwrite.c bn_mp_gcd.c bn_mp_get_double.c bn_mp_get_i32.c \
bn_mp_get_i64.c bn_mp_get_l.c bn_mp_get_ll.c bn_mp_get_mag_u32.c bn_mp_get_mag_u64.c bn_mp_get_mag_ul.c \
bn_mp_get_mag_ull.c bn_mp_grow.c bn_mp_incr.c bn_mp_init.c bn_mp_init_copy.c bn_mp_init_i32.c \
bn_mp_init_i64.c bn_mp_init_l.c bn_mp_init_ll.c bn_mp_init_multi.c bn_mp_init_set.c bn_mp_init_size.c \
bn_mp_init_u32.c bn_mp_init_u64.c bn_mp_init_ul.c bn_mp_init_ull.c bn_mp_invmod.c bn_mp_is_square.c \
bn_mp_iseven.c bn_mp_isodd.c bn_mp_kronecker.c bn_mp_lcm.c bn_mp_log_u32.c bn_mp_lshd.c bn_mp_mod.c \
bn_mp_mod_2d.c bn_mp_mod_d.c bn_mp_montgomery_calc_normalization.c bn_mp_montgomery_reduce.c \
bn_mp_montgomery_setup.c bn_mp_mul.c bn_mp_mul_2.c bn_mp_mul_2d.c bn_mp_mul_d.c bn_mp_mulmod.c bn_mp_neg.c \
bn_mp_or.c bn_mp_pack.c bn_mp_pack_count.c bn_mp_prime_fermat.c bn_mp_prime_frobenius_underwood.c \
bn_mp_prime_is_prime.c bn_mp_prime_miller_rabin.c bn_mp_prime_next_prime.c \
bn_mp_prime_rabin_miller_trials.c bn_mp_prime_rand.c bn_mp_prime_strong_lucas_selfridge.c \
bn_mp_radix_size.c bn_mp_radix_smap.c bn_mp_rand.c bn_mp_read_radix.c bn_mp_reduce.c bn_mp_reduce_2k.c \
bn_mp_reduce_2k_l.c bn_mp_reduce_2k_setup.c bn_mp_reduce_2k_setup_l.c bn_mp_reduce_is_2k.c \
bn_mp_reduce_is_2k_l.c bn_mp_reduce_setup.c bn_mp_root_u32.c bn_mp_rshd.c bn_mp_sbin_size.c bn_mp_set.c \
bn_mp_set_double.c bn_mp_set_i32.c bn_mp_set_i64.c bn_mp_set_l.c bn_mp_set_ll.c bn_mp_set_u32.c \
bn_mp_set_u64.c bn_mp_set_ul.c bn_mp_set_ull.c bn_mp_shrink.c bn_mp_signed_rsh.c bn_mp_sqr.c \
bn_mp_sqrmod.c bn_mp_sqrt.c bn_mp_sqrtmod_prime.c bn_mp_sub.c bn_mp_sub_d.c bn_mp_submod.c \
bn_mp_to_radix.c bn_mp_to_sbin.c bn_mp_to_ubin.c bn_mp_ubin_size.c bn_mp_unpack.c bn_mp_xor.c bn_mp_zero.c \
bn_prime_tab.c bn_s_mp_add.c bn_s_mp_balance_mul.c bn_s_mp_exptmod.c bn_s_mp_exptmod_fast.c \
bn_s_mp_get_bit.c bn_s_mp_invmod_fast.c bn_s_mp_invmod_slow.c bn_s_mp_karatsuba_mul.c \
bn_s_mp_karatsuba_sqr.c bn_s_mp_montgomery_reduce_fast.c bn_s_mp_mul_digs.c bn_s_mp_mul_digs_fast.c \
bn_s_mp_mul_high_digs.c bn_s_mp_mul_high_digs_fast.c bn_s_mp_prime_is_divisible.c \
bn_s_mp_rand_jenkins.c bn_s_mp_rand_platform.c bn_s_mp_reverse.c bn_s_mp_sqr.c bn_s_mp_sqr_fast.c \
bn_s_mp_sub.c bn_s_mp_toom_mul.c bn_s_mp_toom_sqr.c

VPATH += $(LIBTOMMATH_UNPACKNAME)

ifneq ($(CONFIG_LIBTOMMATH_DEMOS),)
CSRCS += shared.c
VPATH += $(LIBTOMMATH_UNPACKNAME)/demo

ifneq ($(CONFIG_LIBTOMMATH_TEST),)
MAINSRC += test.c

PROGNAME += $(CONFIG_LIBTOMMATH_TEST_PROGNAME)
PRIORITY += $(CONFIG_LIBTOMMATH_TEST_PRIORITY)
STACKSIZE += $(CONFIG_LIBTOMMATH_TEST_STACKSIZE)
endif

ifneq ($(CONFIG_LIBTOMMATH_MTEST_OPPONENT),)
MAINSRC += mtest_opponent.c

PROGNAME += $(CONFIG_LIBTOMMATH_MTEST_OPPONENT_PROGNAME)
PRIORITY += $(CONFIG_LIBTOMMATH_MTEST_OPPONENT_PRIORITY)
STACKSIZE += $(CONFIG_LIBTOMMATH_MTEST_OPPONENT_STACKSIZE)
endif

ifneq ($(CONFIG_LIBTOMMATH_TIMING),)
MAINSRC += timing.c

PROGNAME += $(CONFIG_LIBTOMMATH_TIMING_PROGNAME)
PRIORITY += $(CONFIG_LIBTOMMATH_TIMING_PRIORITY)
STACKSIZE += $(CONFIG_LIBTOMMATH_TIMING_STACKSIZE)
endif

endif
# Set up build configuration and environment

CONFIG_LIBTOMMATH_URL ?= "https://github.com/libtom/libtommath/archive"

LIBTOMMATH_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBTOMMATH_VERSION)))
LIBTOMMATH_ZIP = v$(LIBTOMMATH_VERSION).zip

LIBTOMMATH_UNPACKNAME = libtommath
UNPACK ?= unzip -o

$(LIBTOMMATH_ZIP):
	@echo "Downloading: $(LIBTOMMATH_ZIP)"
	$(Q) curl -O -L $(CONFIG_LIBTOMMATH_URL)/$(LIBTOMMATH_ZIP)

$(LIBTOMMATH_UNPACKNAME): $(LIBTOMMATH_ZIP)
	@echo "Unpacking: $(LIBTOMMATH_ZIP) -> $(LIBTOMMATH_UNPACKNAME)"
	$(Q) $(UNPACK) $(LIBTOMMATH_ZIP)
	$(Q) mv	libtommath-$(LIBTOMMATH_VERSION) $(LIBTOMMATH_UNPACKNAME)
	$(Q) touch $(LIBTOMMATH_UNPACKNAME)

context:: $(LIBTOMMATH_UNPACKNAME)

distclean::
	$(call DELDIR, $(LIBTOMMATH_UNPACKNAME))
	$(call DELFILE, $(LIBTOMMATH_ZIP))

include $(APPDIR)/Application.mk
