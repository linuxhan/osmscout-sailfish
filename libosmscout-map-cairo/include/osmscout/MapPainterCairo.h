#ifndef OSMSCOUT_MAP_MAPPAINTERCAIRO_H
#define OSMSCOUT_MAP_MAPPAINTERCAIRO_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/MapCairoFeatures.h>

#if defined(__WIN32__) || defined(WIN32) || defined(__APPLE__)
  #include <cairo.h>
#else
  #include <cairo/cairo.h>
#endif

#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
  #include <pango/pangocairo.h>
#endif

#include <osmscout/private/MapCairoImportExport.h>

#include <osmscout/util/HashMap.h>

#include <osmscout/MapPainter.h>

namespace osmscout {

  class OSMSCOUT_MAP_CAIRO_API MapPainterCairo : public MapPainter
  {
  private:
#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
    typedef PangoFontDescription*          Font;
#else
    typedef cairo_scaled_font_t*           Font;
#endif
    typedef OSMSCOUT_HASHMAP<size_t,Font>  FontMap;   //! Map type for mapping  font sizes to font

    cairo_t                                *draw;     //! The cairo cairo_t for the mask
    std::vector<cairo_surface_t*>          images;    //! vector of cairo surfaces for icons
    std::vector<cairo_pattern_t*>          patterns;  //! cairo pattern structure for patterns
    FontMap                                fonts;     //! Cached scaled font
    double                                 minimumLineWidth; //! Minimum width a line must have to be visible

  private:
    Font GetFont(const MapParameter& parameter,
                 double fontSize);

    void SetLineAttributes(const Color& color,
                           double width,
                           const std::vector<double>& dash);

    void DrawFillStyle(const Projection& projection,
                       const MapParameter& parameter,
                       const FillStyle& fill);

  protected:
    bool HasIcon(const StyleConfig& styleConfig,
                 const MapParameter& parameter,
                 IconStyle& style);

    bool HasPattern(const MapParameter& parameter,
                    const FillStyle& style);

    void GetTextDimension(const MapParameter& parameter,
                          double fontSize,
                          const std::string& text,
                          double& xOff,
                          double& yOff,
                          double& width,
                          double& height);

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const LabelData& label);

    void DrawPlateLabel(const Projection& projection,
                        const MapParameter& parameter,
                        const LabelData& label);

    void DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const LabelStyle& style,
                          const std::string& text,
                          size_t transStart, size_t transEnd);

    void DrawSymbol(const Projection& projection,
                    const MapParameter& parameter,
                    const SymbolRef& symbol,
                    double x, double y);

    void DrawIcon(const IconStyle* style,
                  double x, double y);

    void DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  size_t transStart, size_t transEnd);

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area);

    void DrawArea(const FillStyle& style,
                  const MapParameter& parameter,
                  double x,
                  double y,
                  double width,
                  double height);

  public:
    MapPainterCairo();
    virtual ~MapPainterCairo();


    bool DrawMap(const StyleConfig& styleConfig,
                 const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 cairo_t *draw);
  };
}

#endif
