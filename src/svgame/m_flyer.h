/*
===========================================================================
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2000-2002 Mr. Hyde and Mad Dog

This file is part of Lazarus Quake 2 Mod source code.

Lazarus Quake 2 Mod source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Lazarus Quake 2 Mod source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Lazarus Quake 2 Mod source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// G:\quake2\baseq2\models/monsters/flyer

// This file generated by ModelGen - Do NOT Modify

#define ACTION_nothing			0
#define ACTION_attack1			1
#define ACTION_attack2			2
#define ACTION_run				3
#define ACTION_walk				4

#define FRAME_start01         	0
#define FRAME_start02         	1
#define FRAME_start03         	2
#define FRAME_start04         	3
#define FRAME_start05         	4
#define FRAME_start06         	5
#define FRAME_stop01          	6
#define FRAME_stop02          	7
#define FRAME_stop03          	8
#define FRAME_stop04          	9
#define FRAME_stop05          	10
#define FRAME_stop06          	11
#define FRAME_stop07          	12
#define FRAME_stand01         	13
#define FRAME_stand02         	14
#define FRAME_stand03         	15
#define FRAME_stand04         	16
#define FRAME_stand05         	17
#define FRAME_stand06         	18
#define FRAME_stand07         	19
#define FRAME_stand08         	20
#define FRAME_stand09         	21
#define FRAME_stand10         	22
#define FRAME_stand11         	23
#define FRAME_stand12         	24
#define FRAME_stand13         	25
#define FRAME_stand14         	26
#define FRAME_stand15         	27
#define FRAME_stand16         	28
#define FRAME_stand17         	29
#define FRAME_stand18         	30
#define FRAME_stand19         	31
#define FRAME_stand20         	32
#define FRAME_stand21         	33
#define FRAME_stand22         	34
#define FRAME_stand23         	35
#define FRAME_stand24         	36
#define FRAME_stand25         	37
#define FRAME_stand26         	38
#define FRAME_stand27         	39
#define FRAME_stand28         	40
#define FRAME_stand29         	41
#define FRAME_stand30         	42
#define FRAME_stand31         	43
#define FRAME_stand32         	44
#define FRAME_stand33         	45
#define FRAME_stand34         	46
#define FRAME_stand35         	47
#define FRAME_stand36         	48
#define FRAME_stand37         	49
#define FRAME_stand38         	50
#define FRAME_stand39         	51
#define FRAME_stand40         	52
#define FRAME_stand41         	53
#define FRAME_stand42         	54
#define FRAME_stand43         	55
#define FRAME_stand44         	56
#define FRAME_stand45         	57
#define FRAME_attak101        	58
#define FRAME_attak102        	59
#define FRAME_attak103        	60
#define FRAME_attak104        	61
#define FRAME_attak105        	62
#define FRAME_attak106        	63
#define FRAME_attak107        	64
#define FRAME_attak108        	65
#define FRAME_attak109        	66
#define FRAME_attak110        	67
#define FRAME_attak111        	68
#define FRAME_attak112        	69
#define FRAME_attak113        	70
#define FRAME_attak114        	71
#define FRAME_attak115        	72
#define FRAME_attak116        	73
#define FRAME_attak117        	74
#define FRAME_attak118        	75
#define FRAME_attak119        	76
#define FRAME_attak120        	77
#define FRAME_attak121        	78
#define FRAME_attak201        	79
#define FRAME_attak202        	80
#define FRAME_attak203        	81
#define FRAME_attak204        	82
#define FRAME_attak205        	83
#define FRAME_attak206        	84
#define FRAME_attak207        	85
#define FRAME_attak208        	86
#define FRAME_attak209        	87
#define FRAME_attak210        	88
#define FRAME_attak211        	89
#define FRAME_attak212        	90
#define FRAME_attak213        	91
#define FRAME_attak214        	92
#define FRAME_attak215        	93
#define FRAME_attak216        	94
#define FRAME_attak217        	95
#define FRAME_bankl01         	96
#define FRAME_bankl02         	97
#define FRAME_bankl03         	98
#define FRAME_bankl04         	99
#define FRAME_bankl05         	100
#define FRAME_bankl06         	101
#define FRAME_bankl07         	102
#define FRAME_bankr01         	103
#define FRAME_bankr02         	104
#define FRAME_bankr03         	105
#define FRAME_bankr04         	106
#define FRAME_bankr05         	107
#define FRAME_bankr06         	108
#define FRAME_bankr07         	109
#define FRAME_rollf01         	110
#define FRAME_rollf02         	111
#define FRAME_rollf03         	112
#define FRAME_rollf04         	113
#define FRAME_rollf05         	114
#define FRAME_rollf06         	115
#define FRAME_rollf07         	116
#define FRAME_rollf08         	117
#define FRAME_rollf09         	118
#define FRAME_rollr01         	119
#define FRAME_rollr02         	120
#define FRAME_rollr03         	121
#define FRAME_rollr04         	122
#define FRAME_rollr05         	123
#define FRAME_rollr06         	124
#define FRAME_rollr07         	125
#define FRAME_rollr08         	126
#define FRAME_rollr09         	127
#define FRAME_defens01        	128
#define FRAME_defens02        	129
#define FRAME_defens03        	130
#define FRAME_defens04        	131
#define FRAME_defens05        	132
#define FRAME_defens06        	133
#define FRAME_pain101         	134
#define FRAME_pain102         	135
#define FRAME_pain103         	136
#define FRAME_pain104         	137
#define FRAME_pain105         	138
#define FRAME_pain106         	139
#define FRAME_pain107         	140
#define FRAME_pain108         	141
#define FRAME_pain109         	142
#define FRAME_pain201         	143
#define FRAME_pain202         	144
#define FRAME_pain203         	145
#define FRAME_pain204         	146
#define FRAME_pain301         	147
#define FRAME_pain302         	148
#define FRAME_pain303         	149
#define FRAME_pain304         	150

#define MODEL_SCALE		1.000000
