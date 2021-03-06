/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkPaint.h"
#include "SkPath.h"
#include "SkStream.h"
#include "SkTArray.h"
#include "SkTSort.h"
#include "SkTypeface.h"
#include <stdio.h>

enum {
    kEmSize = 4096
};

static void output_fixed(FILE* out, SkScalar num) {
    SkASSERT(floor(num) == num);
    int hex = (int) num * (65536 / kEmSize);
    fprintf(out, "0x%08x", hex);
}

static void output_scalar(FILE* out, SkScalar num) {
    num /= kEmSize;
    if (num == (int) num) {
       fprintf(out, "%d", (int) num);
    } else {
        SkString str;
        str.printf("%1.6g", num);
        int width = (int) str.size();
        const char* cStr = str.c_str();
        while (cStr[width - 1] == '0') {
            --width;
        }
        str.resize(width);
        fprintf(out, "%sf", str.c_str());
    }
}

static void output_points(FILE* out, const SkPoint* pts, int count) {
    for (int index = 0; index < count; ++index) {
        SkASSERT(floor(pts[index].fX) == pts[index].fX);
        output_scalar(out, pts[index].fX);
        fprintf(out, ", ");
        SkASSERT(floor(pts[index].fY) == pts[index].fY);
        output_scalar(out, pts[index].fY);
        if (index + 1 < count) {
            fprintf(out, ", ");
        }
    }
    fprintf(out, ");\n");
}

const char header[] =
"/*\n"
" * Copyright 2014 Google Inc.\n"
" *\n"
" * Use of this source code is governed by a BSD-style license that can be\n"
" * found in the LICENSE file.\n"
" */\n"
"\n"
"// this was auto-generated by TestFontCreator.cpp\n"
"\n"
"#include \"SkPaint.h\"\n"
"#include \"SkPath.h\"\n"
"#include \"SkTDArray.h\"\n"
"\n"
"namespace sk_tool_utils {\n"
"\n"
"SkPaint::FontMetrics create_font(SkTDArray<SkPath*>& pathArray,\n"
"                                 SkTDArray<SkFixed>& widthArray) {";

int tool_main(int argc, char** argv);
int tool_main(int argc, char** argv) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setTextAlign(SkPaint::kLeft_Align);
    paint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    paint.setTextSize(kEmSize);
    SkString filename("resources/DroidSans.ttf");
    SkAutoTUnref<SkFILEStream> stream(new SkFILEStream(filename.c_str()));
    if (!stream->isValid()) {
        SkDebugf("Could not find resources/Roboto-Regular.ttf.\n");
        return 1;
    }
    SkTypeface* typeface = SkTypeface::CreateFromStream(stream);
    SkSafeUnref(paint.setTypeface(typeface));
    FILE* out = fopen("./out/sk_tool_utils_font.cpp", "w");
    fprintf(out, "%s\n", header);
    fprintf(out, "    SkPath* path;\n");
    for (unsigned short index = 0x20; index < 0x7F; ++index) {
        SkPath path;
        paint.getTextPath((const void*) &index, 2, 0, 0, &path);
        SkPath::RawIter iter(path);
        uint8_t verb;
        SkPoint pts[4];
        fprintf(out, "    path = SkNEW(SkPath); //");
        if (index == '\\') {
            fprintf(out, " blackslash\n");
        } else if (index == ' ') {
            fprintf(out, " space\n");
        } else {
            fprintf(out, " %c\n", (char) index);
        }
        fprintf(out, "    *pathArray.append() = path;\n");
        while ((verb = iter.next(pts)) != SkPath::kDone_Verb) {
            switch (verb) {
                case SkPath::kMove_Verb:
                    fprintf(out, "    path->moveTo(");
                    output_points(out, &pts[0], 1);
                    continue;
                case SkPath::kLine_Verb:
                    fprintf(out, "    path->lineTo(");
                    output_points(out, &pts[1], 1);
                    break;
                case SkPath::kQuad_Verb:
                    fprintf(out, "    path->quadTo(");
                    output_points(out, &pts[1], 2);
                    break;
                case SkPath::kCubic_Verb:
                    fprintf(out, "    path->cubicTo(");
                    output_points(out, &pts[1], 3);
                    break;
                case SkPath::kClose_Verb:
                    fprintf(out, "    path->close();\n");
                    break;
                default:
                    SkDEBUGFAIL("bad verb");
                    return 1;
            }
        }
        SkScalar width;
        SkDEBUGCODE(int charCount =) paint.getTextWidths((const void*) &index, 2, &width);
        SkASSERT(charCount == 1);
        fprintf(out, "    *widthArray.append() = ");
        output_fixed(out, width);
        fprintf(out, ";\n\n");
    }
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);
    fprintf(out, "    SkPaint::FontMetrics metrics = {\n");
    fprintf(out, "        0x%08x, ", metrics.fFlags);
    output_scalar(out, metrics.fTop); fprintf(out, ", ");
    output_scalar(out, metrics.fAscent); fprintf(out, ", ");
    output_scalar(out, metrics.fDescent); fprintf(out, ", ");
    output_scalar(out, metrics.fBottom); fprintf(out, ", ");
    output_scalar(out, metrics.fLeading); fprintf(out, ",\n        ");
    output_scalar(out, metrics.fAvgCharWidth); fprintf(out, ", ");
    output_scalar(out, metrics.fMaxCharWidth); fprintf(out, ", ");
    output_scalar(out, metrics.fXMin); fprintf(out, ", ");
    output_scalar(out, metrics.fXMax); fprintf(out, ",\n        ");
    output_scalar(out, metrics.fXHeight); fprintf(out, ", ");
    output_scalar(out, metrics.fCapHeight); fprintf(out, ", ");
    output_scalar(out, metrics.fUnderlineThickness); fprintf(out, ", ");
    output_scalar(out, metrics.fUnderlinePosition); fprintf(out, "\n    };\n");
    fprintf(out, "    return metrics;\n");
    fprintf(out, "}\n\n}\n");
    fclose(out);
    return 0;
}

#if !defined SK_BUILD_FOR_IOS
int main(int argc, char * const argv[]) {
    return tool_main(argc, (char**) argv);
}
#endif
