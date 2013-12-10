/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//  * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZILLA_GFX_TEXTURECLIENTOGL_H
#define MOZILLA_GFX_TEXTURECLIENTOGL_H

#include "GLContextTypes.h"             // for SharedTextureHandle, etc
#include "gfxTypes.h"
#include "mozilla/Attributes.h"         // for MOZ_OVERRIDE
#include "mozilla/gfx/Point.h"          // for IntSize
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersSurfaces.h"  // for SurfaceDescriptor
#include "mozilla/layers/TextureClient.h"  // for DeprecatedTextureClient, etc

#ifdef USE_SKIA
#include "skia/SkGraphics.h"
#include "skia/GrContext.h"
#include "skia/GrTexture.h"
#endif

namespace mozilla {
namespace layers {

class CompositableForwarder;

/**
 * A TextureClient implementation to share TextureMemory that is already
 * on the GPU, for the OpenGL backend.
 */
class SharedTextureClientOGL : public TextureClient
{
public:
  SharedTextureClientOGL(TextureFlags aFlags);

  ~SharedTextureClientOGL();

  virtual bool IsAllocated() const MOZ_OVERRIDE;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  void InitWith(gl::SharedTextureHandle aHandle,
                gfx::IntSize aSize,
                gl::SharedTextureShareType aShareType,
                bool aInverted = false);

  virtual gfx::IntSize GetSize() const { return mSize; }

  virtual TextureClientData* DropTextureData() MOZ_OVERRIDE
  {
    // XXX - right now the code paths using this are managing the shared texture
    // data, although they should use a TextureClientData for this to ensure that
    // the destruction sequence is race-free.
    MarkInvalid();
    return nullptr;
  }

protected:
  gl::SharedTextureHandle mHandle;
  gfx::IntSize mSize;
  gl::SharedTextureShareType mShareType;
  bool mInverted;
};

class SkiaGLSharedTextureClientOGL : public TextureClient
                                   , public TextureClientDrawTarget
{
public:
  SkiaGLSharedTextureClientOGL(gfx::SurfaceFormat aFormat,
                               TextureFlags aFlags = TEXTURE_FLAGS_DEFAULT);
  virtual ~SkiaGLSharedTextureClientOGL();

  virtual TextureClientDrawTarget* AsTextureClientDrawTarget() MOZ_OVERRIDE { return this; }

  bool Lock(OpenMode aMode) MOZ_OVERRIDE;
  void Unlock() MOZ_OVERRIDE;

  bool ImplementsLocking() const MOZ_OVERRIDE { return true; }
  gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }
  bool IsAllocated() const MOZ_OVERRIDE { return mSize.width != -1 && mSize.height != -1; }

  bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE;

  TextureClientData* DropTextureData() MOZ_OVERRIDE;

  // TextureClientDrawTarget
  TemporaryRef<gfx::DrawTarget> GetAsDrawTarget() MOZ_OVERRIDE;
  gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }
  bool AllocateForSurface(gfx::IntSize aSize,
                          TextureAllocationFlags flags = ALLOC_DEFAULT) MOZ_OVERRIDE;

protected:
  gfx::SurfaceFormat mFormat;
  gfx::IntSize mSize;
  uint32_t mGLTextureID;

  RefPtr<gfx::DrawTarget> mDrawTarget;

  SkRefPtr<GrTexture> mSkiaTexture;
};

class DeprecatedTextureClientSharedOGL : public DeprecatedTextureClient
{
public:
  DeprecatedTextureClientSharedOGL(CompositableForwarder* aForwarder, const TextureInfo& aTextureInfo);
  ~DeprecatedTextureClientSharedOGL() { ReleaseResources(); }

  virtual bool SupportsType(DeprecatedTextureClientType aType) MOZ_OVERRIDE { return aType == TEXTURE_SHARED_GL; }
  virtual bool EnsureAllocated(gfx::IntSize aSize, gfxContentType aType);
  virtual void ReleaseResources();
  virtual gfxContentType GetContentType() MOZ_OVERRIDE { return GFX_CONTENT_COLOR_ALPHA; }

protected:
  gl::GLContext* mGL;
  gfx::IntSize mSize;

  friend class CompositingFactory;
};

// Doesn't own the surface descriptor, so we shouldn't delete it
class DeprecatedTextureClientSharedOGLExternal : public DeprecatedTextureClientSharedOGL
{
public:
  DeprecatedTextureClientSharedOGLExternal(CompositableForwarder* aForwarder, const TextureInfo& aTextureInfo)
    : DeprecatedTextureClientSharedOGL(aForwarder, aTextureInfo)
  {}

  virtual bool SupportsType(DeprecatedTextureClientType aType) MOZ_OVERRIDE { return aType == TEXTURE_SHARED_GL_EXTERNAL; }
  virtual void ReleaseResources() {}
};

class DeprecatedTextureClientStreamOGL : public DeprecatedTextureClient
{
public:
  DeprecatedTextureClientStreamOGL(CompositableForwarder* aForwarder, const TextureInfo& aTextureInfo)
    : DeprecatedTextureClient(aForwarder, aTextureInfo)
  {}
  ~DeprecatedTextureClientStreamOGL() { ReleaseResources(); }

  virtual bool SupportsType(DeprecatedTextureClientType aType) MOZ_OVERRIDE { return aType == TEXTURE_STREAM_GL; }
  virtual bool EnsureAllocated(gfx::IntSize aSize, gfxContentType aType) { return true; }
  virtual void ReleaseResources() { mDescriptor = SurfaceDescriptor(); }
  virtual gfxContentType GetContentType() MOZ_OVERRIDE { return GFX_CONTENT_COLOR_ALPHA; }
};

} // namespace
} // namespace

#endif
