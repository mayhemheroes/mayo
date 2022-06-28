/****************************************************************************
** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "measure_tool.h"

namespace Mayo {

class MeasureShapeTool : public IMeasureTool {
public:
    Span<const GraphicsObjectSelectionMode> selectionModes(MeasureType type) const override;
    bool supports(const GraphicsObjectPtr& object) const override;
    bool supports(MeasureType type) const override;

    Result<gp_Pnt> vertexPosition(const GraphicsOwnerPtr& owner) const override;
    Result<Circle> circle(const GraphicsOwnerPtr& owner) const override;
    Result<MinDistance> minDistance(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const override;
    Result<QuantityLength> length(Span<const GraphicsOwnerPtr> spanOwner) const override;
    Result<QuantityAngle> angle(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const override;
    Result<QuantityArea> surfaceArea(Span<const GraphicsOwnerPtr> spanOwner) const override;
};

} // namespace Mayo
