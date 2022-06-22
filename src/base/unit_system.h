/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "quantity.h"
#include <string_view>

namespace Mayo {

class UnitSystem {
public:
    enum Schema {
        SI,
        ImperialUK
    };

    struct TranslateResult {
        double value;
        const char* strUnit; // UTF8
        double factor;
        constexpr operator double() const { return this->value; }
        constexpr operator bool() const { return this->strUnit != nullptr; }
    };

    template<Unit UNIT>
    static TranslateResult translate(Schema schema, Quantity<UNIT> qty) {
        return UnitSystem::translate(schema, qty.value(), UNIT);
    }
    static TranslateResult translate(Schema schema, double value, Unit unit);
    static TranslateResult parseQuantity(std::string_view strQuantity, Unit* ptrUnit = nullptr);

    static TranslateResult translateLength(QuantityLength length, std::string_view strUnit);
    static TranslateResult translateAngle(QuantityAngle angle, std::string_view strUnit);

    static TranslateResult radians(QuantityAngle angle);
    static TranslateResult degrees(QuantityAngle angle);
    static TranslateResult meters(QuantityLength length);
    static TranslateResult millimeters(QuantityLength length);
    static TranslateResult cubicMillimeters(QuantityVolume volume);
    static TranslateResult millimetersPerSecond(QuantityVelocity speed);
    static TranslateResult seconds(QuantityTime duration);

private:
    UnitSystem() = default;
};

} // namespace Mayo
