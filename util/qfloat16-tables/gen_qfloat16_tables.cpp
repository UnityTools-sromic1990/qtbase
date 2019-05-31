/****************************************************************************
**
** Copyright (C) 2016 by Southwest Research Institute (R)
** Copyright (C) 2019 Intel Corporation.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <stdint.h>
#include <stdio.h>

uint32_t convertmantissa(int32_t i)
{
    uint32_t m = i << 13; // Zero pad mantissa bits
    uint32_t e = 0; // Zero exponent

    while (!(m & 0x00800000)) {   // While not normalized
        e -= 0x00800000;     // Decrement exponent (1<<23)
        m <<= 1;             // Shift mantissa
    }
    m &= ~0x00800000;          // Clear leading 1 bit
    e += 0x38800000;           // Adjust bias ((127-14)<<23)
    return m | e;            // Return combined number
}

// we first build these tables up and then print them out as a separate step in order
// to more closely map the implementation given in the paper.
uint32_t basetable[512];
uint32_t shifttable[512];

int main()
{
    uint32_t i;

    printf("/* This file was generated by gen_qfloat16_tables.cpp */\n\n");
    printf("#include <QtCore/qfloat16.h>\n\n");

    printf("QT_BEGIN_NAMESPACE\n\n");
    printf("#if !defined(__F16C__) && !defined(__ARM_FP16_FORMAT_IEEE)\n\n");

    printf("const quint32 qfloat16::mantissatable[2048] = {\n");
    printf("0,\n");
    for (i = 1; i < 1024; i++)
        printf("0x%XU,\n", convertmantissa(i));
    for (i = 1024; i < 2048; i++)
        printf("0x%XU,\n", 0x38000000U + ((i - 1024) << 13));
    printf("};\n\n");

    printf("const quint32 qfloat16::exponenttable[64] = {\n");
    printf("0,\n");
    for (i = 1; i < 31; i++)
        printf("0x%XU,\n", i << 23);
    printf("0x47800000U,\n");  // 31
    printf("0x80000000U,\n");  // 32
    for (i = 33; i < 63; i++)
        printf("0x%XU,\n", 0x80000000U + ((i - 32) << 23));
    printf("0xC7800000U,\n");  // 63
    printf("};\n\n");

    printf("const quint32 qfloat16::offsettable[64] = {\n");
    printf("0,\n");
    for (i = 1; i < 32; i++)
        printf("1024U,\n");
    printf("0,\n");
    for (i = 33; i < 64; i++)
        printf("1024U,\n");
    printf("};\n\n");

    int32_t e;
    for (i = 0; i < 256; ++i) {
        e = i - 127;
        if (e < -24) {   // Very small numbers map to zero
            basetable[i | 0x000] = 0x0000;
            basetable[i | 0x100] = 0x8000;
            shifttable[i | 0x000] = 24;
            shifttable[i | 0x100] = 24;

        } else if (e < -14) {             // Small numbers map to denorms
            basetable[i | 0x000] = (0x0400 >> (-e - 14));
            basetable[i | 0x100] = (0x0400 >> (-e - 14)) | 0x8000;
            shifttable[i | 0x000] = -e - 1;
            shifttable[i | 0x100] = -e - 1;

        } else if (e <= 15) {            // Normal numbers just lose precision
            basetable[i | 0x000] = ((e + 15) << 10);
            basetable[i | 0x100] = ((e + 15) << 10) | 0x8000;
            shifttable[i | 0x000] = 13;
            shifttable[i | 0x100] = 13;

        } else if (e < 128) {            // Large numbers map to Infinity
            basetable[i | 0x000] = 0x7C00;
            basetable[i | 0x100] = 0xFC00;
            shifttable[i | 0x000] = 24;
            shifttable[i | 0x100] = 24;

        } else {                     // Infinity and NaN's stay Infinity and NaN's
            basetable[i | 0x000] = 0x7C00;
            basetable[i | 0x100] = 0xFC00;
            shifttable[i | 0x000] = 13;
            shifttable[i | 0x100] = 13;
        }
    }

    printf("const quint32 qfloat16::basetable[512] = {\n");
    for (i = 0; i < 512; i++)
        printf("0x%XU,\n", basetable[i]);

    printf("};\n\n");

    printf("const quint32 qfloat16::shifttable[512] = {\n");
    for (i = 0; i < 512; i++)
        printf("0x%XU,\n", shifttable[i]);

    printf("};\n\n");

    printf("#endif // !__F16C__ && !__ARM_FP16_FORMAT_IEEE\n\n");
    printf("QT_END_NAMESPACE\n");
    return 0;
}