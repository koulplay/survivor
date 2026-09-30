#include "world.h"

#include <cassert>

[[nodiscard]] Rectangle CardRect(int i) {
    constexpr float total = 3 * kCardWidth + 2 * kCardGap;
    return {
        (kScreenWidth - total) / 2 + i * (kCardWidth + kCardGap),
        (kScreenHeight - kCardHeight) / 2,
        kCardWidth,
        kCardHeight
    };
}

[[nodiscard]] const EnemyDef &GetEnemyDef(EnemyType type) {
    assert(static_cast<size_t>(type) < static_cast<size_t>(EnemyType::Count));
    return kEnemyDefs[static_cast<size_t>(type)];
}
