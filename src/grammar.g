#########################################################################
# Copyright (c) 2017 AT&T Intellectual Property
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors: John Ellson <john.ellson@gmail.com>
#########################################################################

<ACTIVITY ACT>[REP]
<ACT VERB>[OPT]<= SUBJECT><= DISAMBIG>[OPT]<= ATTRIBUTES>[OPT]<= CONTENTS>[OPT]<= TERMINAL>[OPT]
<VERB TLD>[ALT]<= QRY>
<SUBJECT SAMEAS>[ALT]<= NODENOUN>[ALT]<= EDGENOUN>[ALT]<= SET>
<SET LPN><= NOUNS><= RPN>
<NOUNS NODES>[ALT]<= EDGES>
<NODES NODENOUN><= NODENOUN>[REP]
<EDGES EDGENOUN<= EDGENOUN>[REP]
<ATTRIBUTES LBR><= ATTR>[REP]<= RBR>
<CONTENTS LBE><= ACTIVITY>[OPT]<= RBE>
<TERMINAL SCN>
<NODENOUN NODE>
<EDGENOUN EDGE>[ALT]<= SAMEAS>
<NODE ENDPOINT>
<EDGE LAN><= LEG>[REP]<= RAN>
<LEG SAMEAS>[ALT]<= ENDPOINTSET>[ALT]<= ENDPOINT>
<ENDPOINTSET LPN><= ENDPOINT><= ENDPOINT>[REP]<= RPN>
<ENDPOINT PORT>[ALT]<= FAMILY>[ALT]<= RELATIVE>[ALT]<= SAMEAS>
<SAMEAS EQL>
<FAMILY SIS><= KID>[NWS REP]<= PORT>[NWS OPT]
<RELATIVE MUM><= MUM>[NWS REP]<= SIS><= KID>[NWS REP]<= PORT>[NWS OPT]
<SIS NODEID>
<MUM HAT>
<KID FSL>[NWS]<= NODEID>[NWS]
<PORT CLN>[NWS]<= PORTID>[NWS]
<DISAMBIG TIC>[NWS]<= DISAMBID>[NWS]
<ATTR ATTRID><= VALASSIGN>[OPT]
<VALASSIGN EQL><= VALUE>
<ATTRID IDENTIFIER>
<NODEID IDENTIFIER>
<PORTID IDENTIFIER>
<DISAMBID IDENTIFIER>
<VALUE VSTRING>
IDENTIFIER{ABC}
VSTRING{ABC EQL AST FSL CLN SCN QRY BSL HAT TIC TLD}
BIN{00 01 02 03 04 05 06 07 08 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 7f}
ABC{21 24 25 26 27 2b 2c 2d 2e 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50 51 52 53 54 55 56 57 58 59 5a 5f 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a 7c 80 81 82 83 84 85 86 87 88 89 8a 8b 8c 8d 8e 8f 90 91 92 93 94 95 96 97 98 99 9a 9b 9c 9d 9e 9f a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 aa ab ac ad ae af b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 ba bb bc bd be bf c0 c1 c2 c3 c4 c5 c6 c7 c8 c9 ca cb cc cd ce cf d0 d1 d2 d3 d4 d5 d6 d7 d8 d9 da db dc dd de df e0 e1 e2 e3 e4 e5 e6 e7 e8 e9 ea eb ec ed ee ef f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb fc fd fe ff}
WS{09 0a 0b 0c 0d 20}
LBR{5b}
RBR{5d}
LBE{7b}
RBE{7d}
LPN{28}
RPN{29}
LAN{3c}
RAN{3e}
EQL{3d}
DQT{22}
OCT{23}
AST{2a}
FSL{2f}
CLN{3a}
SCN{3b}
QRY{3f}
BSL{5c}
HAT{5e}
TIC{60}
TLD{7e}
