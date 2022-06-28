/****************************************************************************
** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "measure_type.h"
#include "../base/quantity.h"
#include "../base/span.h"
#include "../graphics/graphics_object_ptr.h"
#include "../graphics/graphics_owner_ptr.h"

#include <gp_Pnt.hxx>
#include <gp_Circ.hxx>
#include <string_view>

namespace Mayo {

class IMeasureTool {
public:
    virtual ~IMeasureTool() = default;

    virtual Span<const GraphicsObjectSelectionMode> selectionModes(MeasureType type) const = 0;
    virtual bool supports(const GraphicsObjectPtr& object) const = 0;
    virtual bool supports(MeasureType type) const = 0;

    template<typename T> struct Result {
        bool isValid = false;
        std::string errorMessage;
        T value;
        Result() = default;
        Result(const T& v) : isValid(true), value(v) {}
        Result(T&& v) : isValid(true), value(std::move(v)) {}
        Result(std::string_view errMsg) : isValid(false), errorMessage(errMsg) {}
    };

    struct MinDistance {
        gp_Pnt pnt1;
        gp_Pnt pnt2;
        gp_Pln preferredPlane;
        QuantityLength value;
    };

    struct Circle {
        gp_Pnt pntAnchor;
        gp_Circ value;
    };

    virtual Result<gp_Pnt> vertexPosition(const GraphicsOwnerPtr& owner) const = 0;
    virtual Result<Circle> circle(const GraphicsOwnerPtr& owner) const = 0;
    virtual Result<MinDistance> minDistance(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const = 0;
    virtual Result<QuantityLength> length(Span<const GraphicsOwnerPtr> spanOwner) const = 0;
    virtual Result<QuantityAngle> angle(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const = 0;
    virtual Result<QuantityArea> surfaceArea(Span<const GraphicsOwnerPtr> spanOwner) const = 0;
};

} // namespace Mayo
