/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2007 Lubos Lunak <l.lunak@kde.org>
Copyright (C) 2010 Martin Gräßlin <kde@martin-graesslin.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include "showpaint.h"

#include <kwinconfig.h>

#ifdef KWIN_HAVE_OPENGL_COMPOSITING
#include <kwinglutils.h>
#endif
#ifdef KWIN_HAVE_XRENDER_COMPOSITING
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#endif

#include <math.h>

#include <qcolor.h>
#include <QVector2D>
#include <QVector4D>

namespace KWin
{

KWIN_EFFECT( showpaint, ShowPaintEffect )

static QColor colors[] = { Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta,
    Qt::yellow, Qt::gray };

ShowPaintEffect::ShowPaintEffect()
    : color_index( 0 )
    , useShader( false )
#ifdef KWIN_HAVE_OPENGL_COMPOSITING
    , vbo( 0 )
    , colorShader( 0 )
#endif
    {
#ifdef KWIN_HAVE_OPENGL_COMPOSITING
    if (effects->compositingType() == OpenGLCompositing) {
        vbo = new GLVertexBuffer(GLVertexBuffer::Stream);
        vbo->setUseColor(true);
        // TODO: use GLPlatform
        if (GLShader::vertexShaderSupported() && GLShader::fragmentShaderSupported()) {
            colorShader = new GLShader(":/resources/scene-color-vertex.glsl", ":/resources/scene-color-fragment.glsl");
            if (colorShader->isValid()) {
                colorShader->bind();
                colorShader->setUniform("displaySize", QVector2D(displayWidth(), displayHeight()));
                colorShader->setUniform("geometry", QVector4D(0, 0, 0, 0));
                colorShader->unbind();
                vbo->setUseShader(true);
                useShader = true;
                kDebug(1212) << "Show Paint Shader is valid";
            } else {
                kDebug(1212) << "Show Paint Shader not valid";
            }
        }
    }
#endif
    }

ShowPaintEffect::~ShowPaintEffect()
{
#ifdef KWIN_HAVE_OPENGL_COMPOSITING
    delete vbo;
    delete colorShader;
#endif
}

void ShowPaintEffect::paintScreen( int mask, QRegion region, ScreenPaintData& data )
    {
    painted = QRegion();
    effects->paintScreen( mask, region, data );
#ifdef KWIN_HAVE_OPENGL_COMPOSITING
    if( effects->compositingType() == OpenGLCompositing)
        paintGL();
#endif
#ifdef KWIN_HAVE_XRENDER_COMPOSITING
    if( effects->compositingType() == XRenderCompositing)
        paintXrender();
#endif
    if( ++color_index == sizeof( colors ) / sizeof( colors[ 0 ] ))
        color_index = 0;
    }

void ShowPaintEffect::paintWindow( EffectWindow* w, int mask, QRegion region, WindowPaintData& data )
    {
    painted |= region;
    effects->paintWindow( w, mask, region, data );
    }

void ShowPaintEffect::paintGL()
    {
#ifdef KWIN_HAVE_OPENGL_COMPOSITING
#ifndef KWIN_HAVE_OPENGLES
    glPushAttrib( GL_CURRENT_BIT | GL_ENABLE_BIT );
#endif
    if (useShader) {
        colorShader->bind();
    }
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    QColor color = colors[ color_index ];
    color.setAlphaF(0.2);
    vbo->setColor(color);
    QVector<float> verts;
    verts.reserve(painted.rects().count()*12);
    foreach (const QRect &r, painted.rects()) {
        verts << r.x() + r.width() << r.y();
        verts << r.x() << r.y();
        verts << r.x() << r.y() + r.height();
        verts << r.x() << r.y() + r.height();
        verts << r.x() + r.width() << r.y() + r.height();
        verts << r.x() + r.width() << r.y();
    }
    vbo->setData(verts.count()/2, 2, verts.data(), NULL);
    vbo->render(GL_TRIANGLES);
    if (useShader) {
        colorShader->unbind();
    }
    glDisable( GL_BLEND );
#ifndef KWIN_HAVE_OPENGLES
    glPopAttrib();
#endif
#endif
    }

void ShowPaintEffect::paintXrender()
    {
#ifdef KWIN_HAVE_XRENDER_COMPOSITING
    XRenderColor col;
    float alpha = 0.2;
    const QColor& color = colors[ color_index ];
    col.alpha = int( alpha * 0xffff );
    col.red = int( alpha * 0xffff * color.red() / 255 );
    col.green = int( alpha * 0xffff * color.green() / 255 );
    col.blue= int( alpha * 0xffff * color.blue() / 255 );
    foreach( const QRect &r, painted.rects())
        XRenderFillRectangle( display(), PictOpOver, effects->xrenderBufferPicture(),
            &col, r.x(), r.y(), r.width(), r.height());
#endif
    }

} // namespace
