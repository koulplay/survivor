#include "world.h"

Rectangle CardRect(int i) {
    constexpr float total = 3 * kCardWidth + 2 * kCardGap;
    return {
        (kScreenWidth - total) / 2 + i * (kCardWidth + kCardGap), (kScreenHeight - kCardHeight) / 2, kCardWidth,
        kCardHeight
    };
}

