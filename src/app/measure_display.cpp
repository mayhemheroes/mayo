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
#include <ElCLib.hxx>
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

std::string BaseMeasureDisplay::text(double value)
{
    const QStringUtils::TextOptions textOpts = AppModule::get()->defaultTextOptions();
    return to_stdString(QStringUtils::text(value, textOpts));
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

MeasureDisplayCircleCenter::MeasureDisplayCircleCenter(const IMeasureTool::Circle& circle)
    : m_circle(circle.value),
      m_gfxText(new AIS_TextLabel),
      m_gfxPoint(new AIS_Point(new Geom_CartesianPoint(m_circle.Location()))),
      m_gfxCircle(new AIS_Circle(new Geom_Circle(m_circle)))
{
    m_gfxText->SetPosition(m_circle.Location());
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

MeasureDisplayCircleDiameter::MeasureDisplayCircleDiameter(const IMeasureTool::Circle& circle)
    : m_circle(circle.value),
      m_gfxDiameterText(new AIS_TextLabel)
{
    m_gfxCircle = new AIS_Circle(new Geom_Circle(m_circle));
    m_gfxCircle->SetColor(Quantity_NOC_BLACK);
    m_gfxCircle->Attributes()->LineAspect()->SetTypeOfLine(Aspect_TOL_SOLID);

    const gp_Pnt otherPntAnchor = diameterOpposedPnt(circle.pntAnchor, m_circle);
    m_gfxDiameter = new AIS_Line(new Geom_CartesianPoint(circle.pntAnchor), new Geom_CartesianPoint(otherPntAnchor));
    m_gfxDiameter->SetWidth(2.5);
    m_gfxDiameter->SetColor(Quantity_NOC_BLACK);
    m_gfxDiameter->Attributes()->LineAspect()->SetTypeOfLine(Aspect_TOL_DOT);

    m_gfxDiameterText->SetPosition(m_circle.Location());
    m_gfxDiameterText->SetColor(m_gfxDiameter->Attributes()->Color());
    //m_gfxDiameterText->SetFont("Arial");
}

void MeasureDisplayCircleDiameter::update(const MeasureConfig& config)
{
    const QuantityLength diameter = 2 * m_circle.Radius() * Quantity_Millimeter;
    const auto trDiameter = UnitSystem::translateLength(diameter, config.strLengthUnit);
    const auto strDiameter = BaseMeasureDisplay::text(trDiameter);
    this->setText(fmt::format(
                      MeasureDisplayI18N::textIdTr("Diameter: {0}{1}"), strDiameter, config.strLengthUnit
                  ));

    m_gfxDiameterText->SetText(to_OccExtString(fmt::format(MeasureDisplayI18N::textIdTr("Ø{0}"), strDiameter)));
}

GraphicsObjectPtr MeasureDisplayCircleDiameter::graphicsObjectAt(int i) const
{
    switch (i) {
    case 0: return m_gfxCircle;
    case 1: return m_gfxDiameter;
    case 2: return m_gfxDiameterText;
    }

    return {};
}

gp_Pnt MeasureDisplayCircleDiameter::diameterOpposedPnt(const gp_Pnt& pntOnCircle, const gp_Circ& circ)
{
    return pntOnCircle.Translated(2 * gp_Vec{ pntOnCircle, circ.Location() });
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
    const auto trDiameter = UnitSystem::translateLength(m_dist.value, config.strLengthUnit);
    m_text = fmt::format(
                MeasureDisplayI18N::textIdTr("Min Distance: {0}{1}"),
                BaseMeasureDisplay::text(trDiameter),
                config.strLengthUnit
             );
    m_gfxLength->SetDisplayUnits(to_OccAsciiString(config.strLengthUnit));
}

GraphicsObjectPtr MeasureDisplayMinDistance::graphicsObjectAt(int i) const
{
    return i == 0 ? m_gfxLength : GraphicsObjectPtr{};
}

} // namespace Mayo
