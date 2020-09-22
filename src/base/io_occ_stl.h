/****************************************************************************
** Copyright (c) 2020, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "io.h"

#include <Poly_Triangulation.hxx>

namespace Mayo {
namespace IO {

class OccStlReader : public Reader {
public:
    bool readFile(const QString& filepath, TaskProgress* progress) override;
    bool transfer(DocumentPtr doc, TaskProgress* progress) override;

private:
    Handle_Poly_Triangulation m_mesh;
    QString m_baseFilename;
};

class OccStlWriter : public Writer {
public:
    bool transfer(Span<const ApplicationItem> appItems, TaskProgress* progress) override;
    bool writeFile(const QString& filepath, TaskProgress* progress) override;

    enum class Format { Ascii, Binary };
    Format targetFormat() const { return m_targetFormat; }
    void setTargetFormat(Format format) { m_targetFormat = format; }

    static std::unique_ptr<PropertyGroup> createParameters(PropertyGroup* parentGroup);
    void applyParameters(const PropertyGroup& params) override;

private:
    Format m_targetFormat = Format::Binary;
    TopoDS_Shape m_shape;
    Handle_Poly_Triangulation m_mesh;
};

} // namespace IO
} // namespace Mayo
