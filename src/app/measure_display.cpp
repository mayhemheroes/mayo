/****************************************************************************
** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "measure_display.h"

#include "app_module.h"
#include "measure_tool.h"
#include "qstring_conv.h"
#include "qstring_utils.h"

#include <AIS_Shape.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Geom_Circle.hxx>

#include <fmt/format.h>

namespace Mayo {

namespace {
struct MeasureDisplayI18N { MAYO_DECLARE_TEXT_ID_FUNCTIONS(Mayo::MeasureDisplayI18N) };
} // namespace

std::string BaseMeasureDisplay::text(const gp_Pnt& pnt, const MeasureConfig& config)
{
    const QStringUtils::TextOptions textOpts = AppModule::get()->defaultTextOptions();
    const QString pntText =
            QString("(%1 %2 %3)%4").arg(
                QStringUtils::text(pnt.X(), textOpts),
                QStringUtils::text(pnt.Y(), textOpts),
                QStringUtils::text(pnt.Z(), textOpts),
                to_QString(config.strLengthUnit)
                );
    return to_stdString(pntText);
}

MeasureDisplayVertex::MeasureDisplayVertex(const gp_Pnt& pnt)
    : m_pnt(pnt),
      m_gfxText(new AIS_TextLabel)
{
    m_gfxText->SetPosition(pnt);
}

void MeasureDisplayVertex::update(const MeasureConfig& config)
{
    this->setText(BaseMeasureDisplay::text(m_pnt, config));
    m_gfxText->SetText(to_OccExtString(this->text()));
}

GraphicsObjectPtr MeasureDisplayVertex::graphicsObjectAt(int i) const
{
    return i == 0 ? m_gfxText : GraphicsObjectPtr{};
}

MeasureDisplayCircleCenter::MeasureDisplayCircleCenter(const gp_Circ& circle)
    : m_circle(circle),
      m_gfxText(new AIS_TextLabel),
      m_gfxPoint(new AIS_Point(new Geom_CartesianPoint(circle.Location()))),
      m_gfxCircle(new AIS_Circle(new Geom_Circle(circle)))
{
    m_gfxText->SetPosition(circle.Location());
}

void MeasureDisplayCircleCenter::update(const MeasureConfig& config)
{
    this->setText(BaseMeasureDisplay::text(m_circle.Location(), config));
    m_gfxText->SetText(to_OccExtString("  " + this->text()));
}

GraphicsObjectPtr MeasureDisplayCircleCenter::graphicsObjectAt(int i) const
{
    switch (i) {
    case 0: return m_gfxPoint;
    case 1: return m_gfxText;
    case 2: return m_gfxCircle;
    }
    return GraphicsObjectPtr{};
}

MeasureDisplayCircleDiameter::MeasureDisplayCircleDiameter(const gp_Circ& circle)
    : m_circle(circle),
      m_gfxDiameter(new PrsDim_DiameterDimension(circle))
{
    m_gfxDiameter->DimensionAspect()->ArrowAspect()->SetZoomable(false);
    m_gfxDiameter->DimensionAspect()->ArrowAspect()->SetLength(0.5);
    m_gfxDiameter->DimensionAspect()->MakeUnitsDisplayed(true);
    m_gfxDiameter->SetModelUnits("mm");
    m_gfxDiameter->SetDisplayUnits("mm");
}

void MeasureDisplayCircleDiameter::update(const MeasureConfig& config)
{
    const QStringUtils::TextOptions textOpts = AppModule::get()->defaultTextOptions();
    const QuantityLength diameter = 2 * m_circle.Radius() * Quantity_Millimeter;
    const auto trDiameter = UnitSystem::translateLength(diameter, config.strLengthUnit);
    this->setText(fmt::format(
                      MeasureDisplayI18N::textIdTr("Diameter: {0}{1}"),
                      to_stdString(QStringUtils::text(trDiameter, textOpts)),
                      config.strLengthUnit
                  ));
    m_gfxDiameter->SetDisplayUnits(to_OccAsciiString(config.strLengthUnit));
}

GraphicsObjectPtr MeasureDisplayCircleDiameter::graphicsObjectAt(int i) const
{
    return i == 0 ? m_gfxDiameter : GraphicsObjectPtr{};
}

MeasureDisplayMinDistance::MeasureDisplayMinDistance(const IMeasureTool::MinDistance& dist)
    : m_dist(dist),
      m_gfxLength(new PrsDim_LengthDimension)
{
    const gp_Pln dimPln((dist.pnt1.XYZ() + dist.pnt2.XYZ()) / 2., gp::DZ());
    m_gfxLength->SetMeasuredGeometry(dist.pnt1, dist.pnt2, dimPln);
    m_gfxLength->SetModelUnits("mm");
    m_gfxLength->SetDisplayUnits("mm");
}

void MeasureDisplayMinDistance::update(const MeasureConfig& config)
{
    const QStringUtils::TextOptions textOpts = AppModule::get()->defaultTextOptions();
    const auto trDiameter = UnitSystem::translateLength(m_dist.distance, config.strLengthUnit);
    m_text = fmt::format(
                MeasureDisplayI18N::textIdTr("Min Distance: {0}{1}"),
                to_stdString(QStringUtils::text(trDiameter, textOpts)),
                config.strLengthUnit
                );
    m_gfxLength->SetDisplayUnits(to_OccAsciiString(config.strLengthUnit));
}

GraphicsObjectPtr MeasureDisplayMinDistance::graphicsObjectAt(int i) const
{
    return i == 0 ? m_gfxLength : GraphicsObjectPtr{};
}

} // namespace Mayo
