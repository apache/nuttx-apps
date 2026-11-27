/****************************************************************************
 * apps/testing/crypto/psa-arch-tests/include/test_entry_list.inc
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 ****************************************************************************/

#ifdef CONFIG_PSA_ARCH_CRYPTO_TESTS
  {201, &test_entry_c001},
  {202, &test_entry_c002},
  {203, &test_entry_c003},
  {204, &test_entry_c004},
  {205, &test_entry_c005},
  {206, &test_entry_c006},
  {207, &test_entry_c007},
  {208, &test_entry_c008},
  {209, &test_entry_c009},
  {210, &test_entry_c010},
  {211, &test_entry_c011},
  {212, &test_entry_c012},
  {213, &test_entry_c013},
  {214, &test_entry_c014},
  {215, &test_entry_c015},
  {216, &test_entry_c016},
  {217, &test_entry_c017},
  {218, &test_entry_c018},
  {219, &test_entry_c019},
  {220, &test_entry_c020},
  {221, &test_entry_c021},
  {222, &test_entry_c022},
  {223, &test_entry_c023},
  {224, &test_entry_c024},
  {225, &test_entry_c025},
  {226, &test_entry_c026},
  {227, &test_entry_c027},
  {228, &test_entry_c028},
  {229, &test_entry_c029},
  {230, &test_entry_c030},
  {231, &test_entry_c031},
  {232, &test_entry_c032},
  {233, &test_entry_c033},
  {234, &test_entry_c034},
  {235, &test_entry_c035},
  {236, &test_entry_c036},
  {237, &test_entry_c037},
  {238, &test_entry_c038},
  // {239, &test_entry_c039},
  {240, &test_entry_c040},
  // {241, &test_entry_c041},
  {242, &test_entry_c042},
  {243, &test_entry_c043},
  {244, &test_entry_c044},
  // {245, &test_entry_c045},
  {246, &test_entry_c046},
  {247, &test_entry_c047},
  {248, &test_entry_c048},
  {249, &test_entry_c049},
  {250, &test_entry_c050},
  {251, &test_entry_c051},
  {252, &test_entry_c052},
  {253, &test_entry_c053},
  {254, &test_entry_c054},
  {255, &test_entry_c055},
  {256, &test_entry_c056},
  {257, &test_entry_c057},
  {258, &test_entry_c058},
  {259, &test_entry_c059},
  {260, &test_entry_c060},
  {261, &test_entry_c061},
  {262, &test_entry_c062},
  {263, &test_entry_c063},

  /* not support suspend and resume psa hash */

  //  {264, &test_entry_c064},
  //  {265, &test_entry_c065},

  /* not support pake interfaces */

  // {266, &test_entry_c066},
  // {267, &test_entry_c067},
  // {268, &test_entry_c068},
  // {269, &test_entry_c069},
  // {270, &test_entry_c070},
  // {271, &test_entry_c071},
  // {272, &test_entry_c072},
  // {273, &test_entry_c073},
  // {274, &test_entry_c074},
  // {275, &test_entry_c075},
  // {276, &test_entry_c076},
  // {277, &test_entry_c077},
  // {278, &test_entry_c078},
  // {279, &test_entry_c079},
#endif
#ifdef CONFIG_PSA_ARCH_ITS_TESTS
  {401, &test_entry_s001},
  // {402, &test_entry_s002},

  /* Memory overflow test, emulator couldn't execute */

  // {403, &test_entry_s003},
  // {404, &test_entry_s004},
  {405, &test_entry_s005},
  {406, &test_entry_s006},
  // {407, &test_entry_s007},
  // {408, &test_entry_s008},
  {409, &test_entry_s009},
  // {410, &test_entry_s010},

  /* Optional Test */

  // {411, &test_entry_s011},
  // {412, &test_entry_s012},
  // {413, &test_entry_s013},
  // {414, &test_entry_s014},
  // {415, &test_entry_s015},
  // {416, &test_entry_s016},
  // {417, &test_entry_s017},
#endif
#ifdef CONFIG_PSA_ARCH_ATTESTATION_TESTS
  {601, &test_entry_a001},
#endif
