/****************************************************************************
** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "measure_tool.h"
#include "../graphics/graphics_object_ptr.h"

#include <AIS_Circle.hxx>
#include <AIS_Point.hxx>
#include <AIS_TextLabel.hxx>
#include <gp_Circ.hxx>
#include <PrsDim_DiameterDimension.hxx>
#include <PrsDim_LengthDimension.hxx>

#include <memory>
#include <string>
#include <string_view>

namespace Mayo {

struct MeasureConfig {
    std::string_view strLengthUnit = "mm";
    std::string_view strAngleUnit = "deg";
};

class IMeasureDisplay {
public:
    virtual ~IMeasureDisplay() = default;
    virtual void update(const MeasureConfig& config) = 0;
    virtual std::string text() const = 0;
    virtual int graphicsObjectsCount() const = 0;
    virtual GraphicsObjectPtr graphicsObjectAt(int i) const = 0;
};

class BaseMeasureDisplay : public IMeasureDisplay {
public:
    std::string text() const override { return m_text; }
    static std::string text(const gp_Pnt& pnt, const MeasureConfig& config);

protected:
    void setText(std::string_view str) { m_text = str; }

private:
    std::string m_text;
};

// --
// -- Vertex
// --
class MeasureDisplayVertex : public BaseMeasureDisplay {
public:
    MeasureDisplayVertex(const gp_Pnt& pnt);
    void update(const MeasureConfig& config) override;
    int graphicsObjectsCount() const override { return 1; }
    GraphicsObjectPtr graphicsObjectAt(int i) const override;

private:
    gp_Pnt m_pnt;
    Handle_AIS_TextLabel m_gfxText;
};

// --
// -- CircleCenter
// --
class MeasureDisplayCircleCenter : public BaseMeasureDisplay {
public:
    MeasureDisplayCircleCenter(const gp_Circ& circle);
    void update(const MeasureConfig& config) override;
    int graphicsObjectsCount() const override { return 3; }
    GraphicsObjectPtr graphicsObjectAt(int i) const override;

private:
    gp_Circ m_circle;
    Handle_AIS_Point m_gfxPoint;
    Handle_AIS_TextLabel m_gfxText;
    Handle_AIS_Circle m_gfxCircle;
};

// --
// -- CircleDiameter
// --
class MeasureDisplayCircleDiameter : public BaseMeasureDisplay {
public:
    MeasureDisplayCircleDiameter(const gp_Circ& circle);
    void update(const MeasureConfig& config) override;
    int graphicsObjectsCount() const override { return 1; }
    GraphicsObjectPtr graphicsObjectAt(int i) const override;

private:
    gp_Circ m_circle;
    std::string m_text;
    Handle_PrsDim_DiameterDimension m_gfxDiameter;
};

// --
// -- MinDistance
// --
class MeasureDisplayMinDistance : public BaseMeasureDisplay {
public:
    MeasureDisplayMinDistance(const IMeasureTool::MinDistance& dist);
    void update(const MeasureConfig& config) override;
    int graphicsObjectsCount() const override { return 1; }
    GraphicsObjectPtr graphicsObjectAt(int i) const override;

private:
    IMeasureTool::MinDistance m_dist;
    std::string m_text;
    Handle_PrsDim_LengthDimension m_gfxLength;
};

} // namespace Mayo
