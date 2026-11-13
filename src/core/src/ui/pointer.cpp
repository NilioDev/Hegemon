// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/pointer.hpp"

#include "hegemon/game_data.hpp"

namespace hegemon {

Pointer::Pointer(GameData& data) : data_(&data) {
    useEntry(kMainMenuEntry);
}

void Pointer::useEntry(std::size_t entry) {
    auto it = sets_.find(entry);
    if (it == sets_.end()) {
        it = sets_.emplace(entry, PointerSet::parse(data_->archive("FONTS.LBX").entry(entry))).first;
    }
    pointers_ = &it->second;
}

void Pointer::move(int x, int y) {
    x_ = x;
    y_ = y;
    visible_ = true;
}

void Pointer::draw(Canvas& canvas) const {
    if (visible_) {
        pointers_->draw(picture_, canvas, x_, y_);
    }
}

} // namespace hegemon
