/**
 *  Copyright 2015 Mike Reed
 */

#include "bench.h"
#include "../include/GCanvas.h"
#include "../include/GBitmap.h"
#include "../include/GColor.h"
#include "../include/GRandom.h"
#include "../include/GRect.h"
#include <string>

#include "bench_pa1.inc"

const GBenchmark::Factory gBenchFactories[] {
    []() -> GBenchmark* { return new RectsBench(false); },
    []() -> GBenchmark* { return new RectsBench(true);  },
    []() -> GBenchmark* {
        return new SingleRectBench({2,2}, GRect::LTRB(-1000, -1000, 1002, 1002), "rect_big");
    },
    []() -> GBenchmark* {
        return new SingleRectBench({1000,1000}, GRect::LTRB(500, 500, 502, 502), "rect_tiny");
    },

    nullptr,
};
