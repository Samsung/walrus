/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FASTA_INPUT_H
#define FASTA_INPUT_H

#define FASTA_ONE_SEQUENCE \
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGA" \
    "TCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACT" \
    "AAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAG" \
    "GCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCG" \
    "CCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAAGGCCGGGCGCGGT" \
    "GGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCA" \
    "GGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAATACAAAAA" \
    "TTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAG" \
    "AATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCCA" \
    "GCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAAGGCCGGGCGCGGTGGCTCACGCCTGT" \
    "AATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGACC" \
    "AGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAATACAAAAATTAGCCGGGCGTG" \
    "GTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACC" \
    "CGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCCAGCCTGGGCGACAG" \
    "AGCGAGACTCCGTCTCAAAAAGGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTT" \
    "TGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACA" \
    "TGGTGAAACCCCGTCTCTACTAAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCT" \
    "GTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGG" \
    "TTGCAGTGAGCCGAGATCGCGCCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGT" \
    "CTCAAAAAGGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGG" \
    "CGGGCGGATCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCG" \
    "TCTCTACTAAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTA" \
    "CTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCG" \
    "AGATCGCGCCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAAGGCCG" \
    "GGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACC" \
    "TGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAA" \
    "TACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGA" \
    "GGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACT" \
    "GCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAAGGCCGGGCGCGGTGGCTC" \
    "ACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGT" \
    "TCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAATACAAAAATTAGC" \
    "CGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCG" \
    "CTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCCAGCCTG" \
    "GGCGACAGAGCGAGACTCCG" 

#define FASTA_TWO_SEQUENCE \
    "cttBtatcatatgctaKggNcataaaSatgtaaaDcDRtBggDtctttataattcBgtcg" \
    "tactDtDagcctatttSVHtHttKtgtHMaSattgWaHKHttttagacatWatgtRgaaa" \
    "NtactMcSMtYtcMgRtacttctWBacgaaatatagScDtttgaagacacatagtVgYgt" \
    "cattHWtMMWcStgttaggKtSgaYaaccWStcgBttgcgaMttBYatcWtgacaYcaga" \
    "gtaBDtRacttttcWatMttDBcatWtatcttactaBgaYtcttgttttttttYaaScYa" \
    "HgtgttNtSatcMtcVaaaStccRcctDaataataStcYtRDSaMtDttgttSagtRRca" \
    "tttHatSttMtWgtcgtatSSagactYaaattcaMtWatttaSgYttaRgKaRtccactt" \
    "tattRggaMcDaWaWagttttgacatgttctacaaaRaatataataaMttcgDacgaSSt" \
    "acaStYRctVaNMtMgtaggcKatcttttattaaaaagVWaHKYagtttttatttaacct" \
    "tacgtVtcVaattVMBcttaMtttaStgacttagattWWacVtgWYagWVRctDattBYt" \
    "gtttaagaagattattgacVatMaacattVctgtBSgaVtgWWggaKHaatKWcBScSWa" \
    "accRVacacaaactaccScattRatatKVtactatatttHttaagtttSKtRtacaaagt" \
    "RDttcaaaaWgcacatWaDgtDKacgaacaattacaRNWaatHtttStgttattaaMtgt" \
    "tgDcgtMgcatBtgcttcgcgaDWgagctgcgaggggVtaaScNatttacttaatgacag" \
    "cccccacatYScaMgtaggtYaNgttctgaMaacNaMRaacaaacaKctacatagYWctg" \
    "ttWaaataaaataRattagHacacaagcgKatacBttRttaagtatttccgatctHSaat" \
    "actcNttMaagtattMtgRtgaMgcataatHcMtaBSaRattagttgatHtMttaaKagg" \
    "YtaaBataSaVatactWtataVWgKgttaaaacagtgcgRatatacatVtHRtVYataSa" \
    "KtWaStVcNKHKttactatccctcatgWHatWaRcttactaggatctataDtDHBttata" \
    "aaaHgtacVtagaYttYaKcctattcttcttaataNDaaggaaaDYgcggctaaWSctBa" \
    "aNtgctggMBaKctaMVKagBaactaWaDaMaccYVtNtaHtVWtKgRtcaaNtYaNacg" \
    "gtttNattgVtttctgtBaWgtaattcaagtcaVWtactNggattctttaYtaaagccgc" \
    "tcttagHVggaYtgtNcDaVagctctctKgacgtatagYcctRYHDtgBattDaaDgccK" \
    "tcHaaStttMcctagtattgcRgWBaVatHaaaataYtgtttagMDMRtaataaggatMt" \
    "ttctWgtNtgtgaaaaMaatatRtttMtDgHHtgtcattttcWattRSHcVagaagtacg" \
    "ggtaKVattKYagactNaatgtttgKMMgYNtcccgSKttctaStatatNVataYHgtNa" \
    "BKRgNacaactgatttcctttaNcgatttctctataScaHtataRagtcRVttacDSDtt" \
    "aRtSatacHgtSKacYagttMHtWataggatgactNtatSaNctataVtttRNKtgRacc" \
    "tttYtatgttactttttcctttaaacatacaHactMacacggtWataMtBVacRaSaatc" \
    "cgtaBVttccagccBcttaRKtgtgcctttttRtgtcagcRttKtaaacKtaaatctcac" \
    "aattgcaNtSBaaccgggttattaaBcKatDagttactcttcattVtttHaaggctKKga" \
    "tacatcBggScagtVcacattttgaHaDSgHatRMaHWggtatatRgccDttcgtatcga" \
    "aacaHtaagttaRatgaVacttagattVKtaaYttaaatcaNatccRttRRaMScNaaaD" \
    "gttVHWgtcHaaHgacVaWtgttScactaagSgttatcttagggDtaccagWattWtRtg" \
    "ttHWHacgattBtgVcaYatcggttgagKcWtKKcaVtgaYgWctgYggVctgtHgaNcV" \
    "taBtWaaYatcDRaaRtSctgaHaYRttagatMatgcatttNattaDttaattgttctaa" \
    "ccctcccctagaWBtttHtBccttagaVaatMcBHagaVcWcagBVttcBtaYMccagat" \
    "gaaaaHctctaacgttagNWRtcggattNatcRaNHttcagtKttttgWatWttcSaNgg" \
    "gaWtactKKMaacatKatacNattgctWtatctaVgagctatgtRaHtYcWcttagccaa" \
    "tYttWttaWSSttaHcaaaaagVacVgtaVaRMgattaVcDactttcHHggHRtgNcctt" \
    "tYatcatKgctcctctatVcaaaaKaaaagtatatctgMtWtaaaacaStttMtcgactt" \
    "taSatcgDataaactaaacaagtaaVctaggaSccaatMVtaaSKNVattttgHccatca" \
    "cBVctgcaVatVttRtactgtVcaattHgtaaattaaattttYtatattaaRSgYtgBag" \
    "aHSBDgtagcacRHtYcBgtcacttacactaYcgctWtattgSHtSatcataaatataHt" \
    "cgtYaaMNgBaatttaRgaMaatatttBtttaaaHHKaatctgatWatYaacttMctctt" \
    "ttVctagctDaaagtaVaKaKRtaacBgtatccaaccactHHaagaagaaggaNaaatBW" \
    "attccgStaMSaMatBttgcatgRSacgttVVtaaDMtcSgVatWcaSatcttttVatag" \
    "ttactttacgatcaccNtaDVgSRcgVcgtgaacgaNtaNatatagtHtMgtHcMtagaa" \
    "attBgtataRaaaacaYKgtRccYtatgaagtaataKgtaaMttgaaRVatgcagaKStc" \
    "tHNaaatctBBtcttaYaBWHgtVtgacagcaRcataWctcaBcYacYgatDgtDHccta" 

#define FASTA_THREE_SEQUENCE \
    "aacacttcaccaggtatcgtgaaggctcaagattacccagagaacctttgcaatataaga" \
    "atatgtatgcagcattaccctaagtaattatattctttttctgactcaaagtgacaagcc" \
    "ctagtgtatattaaatcggtatatttgggaaattcctcaaactatcctaatcaggtagcc" \
    "atgaaagtgatcaaaaaagttcgtacttataccatacatgaattctggccaagtaaaaaa" \
    "tagattgcgcaaaattcgtaccttaagtctctcgccaagatattaggatcctattactca" \
    "tatcgtgtttttctttattgccgccatccccggagtatctcacccatccttctcttaaag" \
    "gcctaatattacctatgcaaataaacatatattgttgaaaattgagaacctgatcgtgat" \
    "tcttatgtgtaccatatgtatagtaatcacgcgactatatagtgctttagtatcgcccgt" \
    "gggtgagtgaatattctgggctagcgtgagatagtttcttgtcctaatatttttcagatc" \
    "gaatagcttctatttttgtgtttattgacatatgtcgaaactccttactcagtgaaagtc" \
    "atgaccagatccacgaacaatcttcggaatcagtctcgttttacggcggaatcttgagtc" \
    "taacttatatcccgtcgcttactttctaacaccccttatgtatttttaaaattacgttta" \
    "ttcgaacgtacttggcggaagcgttattttttgaagtaagttacattgggcagactcttg" \
    "acattttcgatacgactttctttcatccatcacaggactcgttcgtattgatatcagaag" \
    "ctcgtgatgattagttgtcttctttaccaatactttgaggcctattctgcgaaatttttg" \
    "ttgccctgcgaacttcacataccaaggaacacctcgcaacatgccttcatatccatcgtt" \
    "cattgtaattcttacacaatgaatcctaagtaattacatccctgcgtaaaagatggtagg" \
    "ggcactgaggatatattaccaagcatttagttatgagtaatcagcaatgtttcttgtatt" \
    "aagttctctaaaatagttacatcgtaatgttatctcgggttccgcgaataaacgagatag" \
    "attcattatatatggccctaagcaaaaacctcctcgtattctgttggtaattagaatcac" \
    "acaatacgggttgagatattaattatttgtagtacgaagagatataaaaagatgaacaat" \
    "tactcaagtcaagatgtatacgggatttataataaaaatcgggtagagatctgctttgca" \
    "attcagacgtgccactaaatcgtaatatgtcgcgttacatcagaaagggtaactattatt" \
    "aattaataaagggcttaatcactacatattagatcttatccgatagtcttatctattcgt" \
    "tgtatttttaagcggttctaattcagtcattatatcagtgctccgagttctttattattg" \
    "ttttaaggatgacaaaatgcctcttgttataacgctgggagaagcagactaagagtcgga" \
    "gcagttggtagaatgaggctgcaaaagacggtctcgacgaatggacagactttactaaac" \
    "caatgaaagacagaagtagagcaaagtctgaagtggtatcagcttaattatgacaaccct" \
    "taatacttccctttcgccgaatactggcgtggaaaggttttaaaagtcgaagtagttaga" \
    "ggcatctctcgctcataaataggtagactactcgcaatccaatgtgactatgtaatactg" \
    "ggaacatcagtccgcgatgcagcgtgtttatcaaccgtccccactcgcctggggagacat" \
    "gagaccacccccgtggggattattagtccgcagtaatcgactcttgacaatccttttcga" \
    "ttatgtcatagcaatttacgacagttcagcgaagtgactactcggcgaaatggtattact" \
    "aaagcattcgaacccacatgaatgtgattcttggcaatttctaatccactaaagcttttc" \
    "cgttgaatctggttgtagatatttatataagttcactaattaagatcacggtagtatatt" \
    "gatagtgatgtctttgcaagaggttggccgaggaatttacggattctctattgatacaat" \
    "ttgtctggcttataactcttaaggctgaaccaggcgtttttagacgacttgatcagctgt" \
    "tagaatggtttggactccctctttcatgtcagtaacatttcagccgttattgttacgata" \
    "tgcttgaacaatattgatctaccacacacccatagtatattttataggtcatgctgttac" \
    "ctacgagcatggtattccacttcccattcaatgagtattcaacatcactagcctcagaga" \
    "tgatgacccacctctaataacgtcacgttgcggccatgtgaaacctgaacttgagtagac" \
    "gatatcaagcgctttaaattgcatataacatttgagggtaaagctaagcggatgctttat" \
    "ataatcaatactcaataataagatttgattgcattttagagttatgacacgacatagttc" \
    "actaacgagttactattcccagatctagactgaagtactgatcgagacgatccttacgtc" \
    "gatgatcgttagttatcgacttaggtcgggtctctagcggtattggtacttaaccggaca" \
    "ctatactaataacccatgatcaaagcataacagaatacagacgataatttcgccaacata" \
    "tatgtacagaccccaagcatgagaagctcattgaaagctatcattgaagtcccgctcaca" \
    "atgtgtcttttccagacggtttaactggttcccgggagtcctggagtttcgacttacata" \
    "aatggaaacaatgtattttgctaatttatctatagcgtcatttggaccaatacagaatat" \
    "tatgttgcctagtaatccactataacccgcaagtgctgatagaaaatttttagacgattt" \
    "ataaatgccccaagtatccctcccgtgaatcctccgttatactaattagtattcgttcat" \
    "acgtataccgcgcatatatgaacatttggcgataaggcgcgtgaattgttacgtgacaga" \
    "gatagcagtttcttgtgatatggttaacagacgtacatgaagggaaactttatatctata" \
    "gtgatgcttccgtagaaataccgccactggtctgccaatgatgaagtatgtagctttagg" \
    "tttgtactatgaggctttcgtttgtttgcagagtataacagttgcgagtgaaaaaccgac" \
    "gaatttatactaatacgctttcactattggctacaaaatagggaagagtttcaatcatga" \
    "gagggagtatatggatgctttgtagctaaaggtagaacgtatgtatatgctgccgttcat" \
    "tcttgaaagatacataagcgataagttacgacaattataagcaacatccctaccttcgta" \
    "acgatttcactgttactgcgcttgaaatacactatggggctattggcggagagaagcaga" \
    "tcgcgccgagcatatacgagacctataatgttgatgatagagaaggcgtctgaattgata" \
    "catcgaagtacactttctttcgtagtatctctcgtcctctttctatctccggacacaaga" \
    "attaagttatatatatagagtcttaccaatcatgttgaatcctgattctcagagttcttt" \
    "ggcgggccttgtgatgactgagaaacaatgcaatattgctccaaatttcctaagcaaatt" \
    "ctcggttatgttatgttatcagcaaagcgttacgttatgttatttaaatctggaatgacg" \
    "gagcgaagttcttatgtcggtgtgggaataattcttttgaagacagcactccttaaataa" \
    "tatcgctccgtgtttgtatttatcgaatgggtctgtaaccttgcacaagcaaatcggtgg" \
    "tgtatatatcggataacaattaatacgatgttcatagtgacagtatactgatcgagtcct" \
    "ctaaagtcaattacctcacttaacaatctcattgatgttgtgtcattcccggtatcgccc" \
    "gtagtatgtgctctgattgaccgagtgtgaaccaaggaacatctactaatgcctttgtta" \
    "ggtaagatctctctgaattccttcgtgccaacttaaaacattatcaaaatttcttctact" \
    "tggattaactacttttacgagcatggcaaattcccctgtggaagacggttcattattatc" \
    "ggaaaccttatagaaattgcgtgttgactgaaattagatttttattgtaagagttgcatc" \
    "tttgcgattcctctggtctagcttccaatgaacagtcctcccttctattcgacatcgggt" \
    "ccttcgtacatgtctttgcgatgtaataattaggttcggagtgtggccttaatgggtgca" \
    "actaggaatacaacgcaaatttgctgacatgatagcaaatcggtatgccggcaccaaaac" \
    "gtgctccttgcttagcttgtgaatgagactcagtagttaaataaatccatatctgcaatc" \
    "gattccacaggtattgtccactatctttgaactactctaagagatacaagcttagctgag" \
    "accgaggtgtatatgactacgctgatatctgtaaggtaccaatgcaggcaaagtatgcga" \
    "gaagctaataccggctgtttccagctttataagattaaaatttggctgtcctggcggcct" \
    "cagaattgttctatcgtaatcagttggttcattaattagctaagtacgaggtacaactta" \
    "tctgtcccagaacagctccacaagtttttttacagccgaaacccctgtgtgaatcttaat" \
    "atccaagcgcgttatctgattagagtttacaactcagtattttatcagtacgttttgttt" \
    "ccaacattacccggtatgacaaaatgacgccacgtgtcgaataatggtctgaccaatgta" \
    "ggaagtgaaaagataaatat"


#define FASTA_ONE_INPUT \
    ">ONE Homo sapiens alu\n" \
    FASTA_ONE_SEQUENCE \
    "\n"

#define FASTA_TWO_INPUT \
    ">TWO IUB ambiguity codes\n" \
    FASTA_TWO_SEQUENCE \
    "\n"

#define FASTA_THREE_INPUT \
    ">THREE Homo sapiens frequency\n" \
    FASTA_THREE_SEQUENCE 

#endif

