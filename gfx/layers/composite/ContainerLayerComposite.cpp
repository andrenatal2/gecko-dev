/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ContainerLayerComposite.h"
#include <algorithm>                    // for min
#include "FrameMetrics.h"               // for FrameMetrics
#include "Units.h"                      // for LayerRect, LayerPixel, etc
#include "gfx2DGlue.h"                  // for ToMatrix4x4
#include "gfxPrefs.h"                   // for gfxPrefs
#include "gfxUtils.h"                   // for gfxUtils, etc
#include "mozilla/Assertions.h"         // for MOZ_ASSERT, etc
#include "mozilla/RefPtr.h"             // for RefPtr
#include "mozilla/UniquePtr.h"          // for UniquePtr
#include "mozilla/gfx/BaseRect.h"       // for BaseRect
#include "mozilla/gfx/Matrix.h"         // for Matrix4x4
#include "mozilla/gfx/Point.h"          // for Point, IntPoint
#include "mozilla/gfx/Rect.h"           // for IntRect, Rect
#include "mozilla/layers/Compositor.h"  // for Compositor, etc
#include "mozilla/layers/CompositorTypes.h"  // for DiagnosticFlags::CONTAINER
#include "mozilla/layers/Effects.h"     // for Effect, EffectChain, etc
#include "mozilla/layers/TextureHost.h"  // for CompositingRenderTarget
#include "mozilla/layers/AsyncPanZoomController.h"  // for AsyncPanZoomController
#include "mozilla/mozalloc.h"           // for operator delete, etc
#include "nsAutoPtr.h"                  // for nsRefPtr
#include "nsDebug.h"                    // for NS_ASSERTION
#include "nsISupportsImpl.h"            // for MOZ_COUNT_CTOR, etc
#include "nsISupportsUtils.h"           // for NS_ADDREF, NS_RELEASE
#include "nsPoint.h"                    // for nsIntPoint
#include "nsRect.h"                     // for nsIntRect
#include "nsRegion.h"                   // for nsIntRegion
#include "nsTArray.h"                   // for nsAutoTArray
#include "TextRenderer.h"               // for TextRenderer
#include <vector>

namespace mozilla {
namespace layers {

using namespace gfx;

/**
 * Returns a rectangle of content painted opaquely by aLayer. Very consertative;
 * bails by returning an empty rect in any tricky situations.
 */
static nsIntRect
GetOpaqueRect(Layer* aLayer)
{
  nsIntRect result;
  gfx::Matrix matrix;
  bool is2D = aLayer->GetBaseTransform().Is2D(&matrix);

  // Just bail if there's anything difficult to handle.
  if (!is2D || aLayer->GetMaskLayer() ||
    aLayer->GetEffectiveOpacity() != 1.0f ||
    matrix.HasNonIntegerTranslation()) {
    return result;
  }

  if (aLayer->GetContentFlags() & Layer::CONTENT_OPAQUE) {
    result = aLayer->GetEffectiveVisibleRegion().GetLargestRectangle();
  } else {
    // Drill down into RefLayers because that's what we particularly care about;
    // layer construction for aLayer will not have known about the opaqueness
    // of any RefLayer subtrees.
    RefLayer* refLayer = aLayer->AsRefLayer();
    if (refLayer && refLayer->GetFirstChild()) {
      result = GetOpaqueRect(refLayer->GetFirstChild());
    }
  }

  // Translate our opaque region to cover the child
  gfx::Point point = matrix.GetTranslation();
  result.MoveBy(static_cast<int>(point.x), static_cast<int>(point.y));

  const nsIntRect* clipRect = aLayer->GetEffectiveClipRect();
  if (clipRect) {
    result.IntersectRect(result, *clipRect);
  }

  return result;
}

struct LayerVelocityUserData : public LayerUserData {
public:
  LayerVelocityUserData() {
    MOZ_COUNT_CTOR(LayerVelocityUserData);
  }
  ~LayerVelocityUserData() {
    MOZ_COUNT_DTOR(LayerVelocityUserData);
  }

  struct VelocityData {
    VelocityData(TimeStamp frameTime, int scrollX, int scrollY)
      : mFrameTime(frameTime)
      , mPoint(scrollX, scrollY)
    {}

    TimeStamp mFrameTime;
    gfx::Point mPoint;
  };
  std::vector<VelocityData> mData;
};

static gfx::Point GetScrollData(Layer* aLayer) {
  gfx::Matrix matrix;
  if (aLayer->GetLocalTransform().Is2D(&matrix)) {
    return matrix.GetTranslation();
  }

  gfx::Point origin;
  return origin;
}

static void DrawLayerInfo(const nsIntRect& aClipRect,
                          LayerManagerComposite* aManager,
                          Layer* aLayer)
{

  if (aLayer->GetType() == Layer::LayerType::TYPE_CONTAINER) {
    // XXX - should figure out a way to render this, but for now this
    // is hard to do, since it will often get superimposed over the first
    // child of the layer, which is bad.
    return;
  }

  std::stringstream ss;
  aLayer->PrintInfo(ss, "");

  nsIntRegion visibleRegion = aLayer->GetVisibleRegion();

  uint32_t maxWidth = std::min<uint32_t>(visibleRegion.GetBounds().width, 500);

  nsIntPoint topLeft = visibleRegion.GetBounds().TopLeft();
  aManager->GetTextRenderer()->RenderText(ss.str().c_str(), gfx::IntPoint(topLeft.x, topLeft.y),
                                          aLayer->GetEffectiveTransform(), 16,
                                          maxWidth);

}

static LayerVelocityUserData* GetVelocityData(Layer* aLayer) {
  static char sLayerVelocityUserDataKey;
  void* key = reinterpret_cast<void*>(&sLayerVelocityUserDataKey);
  if (!aLayer->HasUserData(key)) {
    LayerVelocityUserData* newData = new LayerVelocityUserData();
    aLayer->SetUserData(key, newData);
  }

  return static_cast<LayerVelocityUserData*>(aLayer->GetUserData(key));
}

static void DrawVelGraph(const nsIntRect& aClipRect,
                         LayerManagerComposite* aManager,
                         Layer* aLayer) {
  Compositor* compositor = aManager->GetCompositor();
  gfx::Rect clipRect(aClipRect.x, aClipRect.y,
                     aClipRect.width, aClipRect.height);

  TimeStamp now = TimeStamp::Now();
  LayerVelocityUserData* velocityData = GetVelocityData(aLayer);

  if (velocityData->mData.size() >= 1 &&
    now > velocityData->mData[velocityData->mData.size() - 1].mFrameTime +
      TimeDuration::FromMilliseconds(200)) {
    // clear stale data
    velocityData->mData.clear();
  }

  const gfx::Point layerTransform = GetScrollData(aLayer);
  velocityData->mData.push_back(
    LayerVelocityUserData::VelocityData(now,
      static_cast<int>(layerTransform.x), static_cast<int>(layerTransform.y)));

  // TODO: dump to file
  // XXX: Uncomment these lines to enable ScrollGraph logging. This is
  //      useful for HVGA phones or to output the data to accurate
  //      graphing software.
  // printf_stderr("ScrollGraph (%p): %f, %f\n",
  // aLayer, layerTransform.x, layerTransform.y);

  // Keep a circular buffer of 100.
  size_t circularBufferSize = 100;
  if (velocityData->mData.size() > circularBufferSize) {
    velocityData->mData.erase(velocityData->mData.begin());
  }

  if (velocityData->mData.size() == 1) {
    return;
  }

  // Clear and disable the graph when it's flat
  for (size_t i = 1; i < velocityData->mData.size(); i++) {
    if (velocityData->mData[i - 1].mPoint != velocityData->mData[i].mPoint) {
      break;
    }
    if (i == velocityData->mData.size() - 1) {
      velocityData->mData.clear();
      return;
    }
  }

  if (aLayer->GetEffectiveVisibleRegion().GetBounds().width < 300 ||
      aLayer->GetEffectiveVisibleRegion().GetBounds().height < 300) {
    // Don't want a graph for smaller layers
    return;
  }

  aManager->SetDebugOverlayWantsNextFrame(true);

  const Matrix4x4& transform = aLayer->GetEffectiveTransform();
  nsIntRect bounds = aLayer->GetEffectiveVisibleRegion().GetBounds();
  IntSize graphSize = IntSize(200, 100);
  Rect graphRect = Rect(bounds.x, bounds.y, graphSize.width, graphSize.height);

  RefPtr<DrawTarget> dt = aManager->CreateDrawTarget(graphSize, SurfaceFormat::B8G8R8A8);
  dt->FillRect(Rect(0, 0, graphSize.width, graphSize.height),
               ColorPattern(Color(0.2f,0,0,1)));

  int yScaleFactor = 3;
  Point prev = Point(0,0);
  bool first = true;
  for (int32_t i = (int32_t)velocityData->mData.size() - 2; i >= 0; i--) {
    const gfx::Point& p1 = velocityData->mData[i+1].mPoint;
    const gfx::Point& p2 = velocityData->mData[i].mPoint;
    int vel = sqrt((p1.x - p2.x) * (p1.x - p2.x) +
                   (p1.y - p2.y) * (p1.y - p2.y));
    Point next = Point(graphRect.width / circularBufferSize * i,
                       graphRect.height - vel/yScaleFactor);
    if (first) {
      first = false;
    } else {
      dt->StrokeLine(prev, next, ColorPattern(Color(0,1,0,1)));
    }
    prev = next;
  }

  RefPtr<DataTextureSource> textureSource = compositor->CreateDataTextureSource();
  RefPtr<SourceSurface> snapshot = dt->Snapshot();
  RefPtr<DataSourceSurface> data = snapshot->GetDataSurface();
  textureSource->Update(data);

  EffectChain effectChain;
  effectChain.mPrimaryEffect = CreateTexturedEffect(SurfaceFormat::B8G8R8A8,
                                                    textureSource,
                                                    Filter::POINT,
                                                    true);

  compositor->DrawQuad(graphRect,
                       clipRect,
                       effectChain,
                       1.0f,
                       transform);
}

static void PrintUniformityInfo(Layer* aLayer)
{

  if(Layer::TYPE_CONTAINER != aLayer->GetType()) {
    return;
  }

  // Don't want to print a log for smaller layers
  if (aLayer->GetEffectiveVisibleRegion().GetBounds().width < 300 ||
      aLayer->GetEffectiveVisibleRegion().GetBounds().height < 300) {
    return;
  }

  FrameMetrics frameMetrics = aLayer->AsContainerLayer()->GetFrameMetrics();
  LayerIntPoint scrollOffset = RoundedToInt(frameMetrics.GetScrollOffsetInLayerPixels());
  const gfx::Point layerTransform = GetScrollData(aLayer);
  gfx::Point layerScroll;
  layerScroll.x = scrollOffset.x - layerTransform.x;
  layerScroll.y = scrollOffset.y - layerTransform.y;

  printf_stderr("UniformityInfo Layer_Move %llu %p %f, %f\n",
    TimeStamp::Now(), aLayer, layerScroll.x, layerScroll.y);
}

/* all of the per-layer prepared data we need to maintain */
struct PreparedLayer
{
  PreparedLayer(LayerComposite *aLayer, nsIntRect aClipRect, bool aRestoreVisibleRegion, nsIntRegion &aVisibleRegion) :
    mLayer(aLayer), mClipRect(aClipRect), mRestoreVisibleRegion(aRestoreVisibleRegion), mSavedVisibleRegion(aVisibleRegion) {}
  LayerComposite* mLayer;
  nsIntRect mClipRect;
  bool mRestoreVisibleRegion;
  nsIntRegion mSavedVisibleRegion;
};

template<class ContainerT> void
ContainerRenderVR(ContainerT* aContainer,
                  LayerManagerComposite* aManager,
                  const nsIntRect& aClipRect,
                  gfx::VRHMDInfo* aHMD)
{
  Compositor* compositor = aManager->GetCompositor();

  // XXX if we only have one child, and if that child is a CanvasLayer (or something else that we can
  // directly render, such as an ImageLayer), and it has CONTENT_NATIVE_VR set, we should be able
  // to skip basically all of this, and just render it with the VR distortion effect.

  RefPtr<CompositingRenderTarget> surface;

  nsIntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();
  float opacity = aContainer->GetEffectiveOpacity();

  gfx::IntRect surfaceRect = gfx::IntRect(visibleRect.x, visibleRect.y,
                                          visibleRect.width, visibleRect.height);

  // make sure that our intermediate surfaces don't exceed the max texture size.
  // XXX these should be based on the HMDInfo's SuggestedEyeResolution()!
  // XXX we need to support vertically-oriented eye stacking,
  // we'll want to figure out how the eye rendering should look like.
  // our full screen will be tall and narrow, so we'll need to create
  // a proper sized surface

  int32_t maxTextureSize = compositor->GetMaxTextureSize();
  int32_t eyeWidth = surfaceRect.width;
  surfaceRect.width = std::min(maxTextureSize, eyeWidth * 2);
  surfaceRect.height = std::min(maxTextureSize, surfaceRect.height);

  // create the one compositing surface for both eyes
  surface = compositor->CreateRenderTarget(surfaceRect, INIT_MODE_CLEAR);
  if (!surface) {
    NS_WARNING("failed to create render target for VR eye intermediate");
    return;
  }

  compositor->PushRenderTarget(surface);

  nsAutoTArray<Layer*, 12> children;
  aContainer->SortChildrenBy3DZOrder(children);

  /**
   * Render this container's contents.  We first render the "native VR" layers, stretching them
   * across the full VR area.  Then we render each layer twice, once for each eye,
   * with different transforms.
   */
  gfx::Matrix4x4 origTransform(aContainer->mEffectiveTransform);

  nsIntRect surfaceClipRect(0, 0, surfaceRect.width, surfaceRect.height);
  for (uint32_t i = 0; i < children.Length(); i++) {
    LayerComposite* layerToRender = static_cast<LayerComposite*>(children.ElementAt(i)->ImplData());
    Layer* layer = layerToRender->GetLayer();
    uint32_t contentFlags = layer->GetContentFlags();

    if (layer->GetEffectiveVisibleRegion().IsEmpty() &&
        !layer->AsContainerLayer()) {
      continue;
    }

    // XXX this is not the right calculation I don't think.  The manager world transform
    // should not be relevant here if we're going to apply a VR distortion.
    nsIntRect clipRect = layer->
      CalculateScissorRect(surfaceClipRect, &aManager->GetWorldTransform());
    if (clipRect.IsEmpty()) {
      continue;
    }

    if (contentFlags & Layer::CONTENT_PRESERVE_3D &&
        !(contentFlags & Layer::CONTENT_NATIVE_VR))
    {
      continue;
    }

    layerToRender->Prepare(surfaceClipRect);
    layerToRender->RenderLayer(surfaceClipRect);
  }

  /*
   * Then the non-native-VR layers.
   * XXX rendering them twice for each eye causes the Thebes layers
   * to also render their content twice, which is unnecessary and expensive.  We need to fix
   * that, so that they can reuse their previous texture for the second rendering.
   */

  nsIntRect eyeClipRect(0, 0, eyeWidth, surfaceRect.height);
  for (uint32_t eye = 0; eye < 2; ++eye) {
    // set up the viewport for the proper rect for this eye.
    // XXX fix this for vertical displays
    // XXX There is no matrix.Translate that takes a Point?!?!?!
    gfx::Point3D eyeTranslation = aHMD->GetEyeTranslation(eye);
    gfx::IntRect eyeRect(eye * eyeWidth, 0, eyeWidth, surfaceRect.height);
    gfx::Matrix4x4 eyeProjection = aHMD->GetEyeProjectionMatrix(eye);

    compositor->PrepareViewport3D(eyeRect, gfx::Matrix(), eyeProjection);

    gfx::Matrix4x4 eyeTransform = gfx::Matrix4x4::Translation(eyeTranslation) * origTransform;

    // make sure mEffectiveTransform is up to date for traversal
    aContainer->mEffectiveTransform = eyeTransform;
    aContainer->ComputeEffectiveTransformsForChildren(eyeTransform);

    for (uint32_t i = 0; i < children.Length(); i++) {
      LayerComposite* layerToRender = static_cast<LayerComposite*>(children.ElementAt(i)->ImplData());
      Layer* layer = layerToRender->GetLayer();
      uint32_t contentFlags = layer->GetContentFlags();

#if 0
      printf_stderr("VR Container, child %d type %s flags (0x%04x): PRESERVE_3D %s NATIVE_VR %s OPAQUE %s\n",
                    i, layer->Name(), contentFlags,
                    contentFlags & Layer::CONTENT_PRESERVE_3D ? "YES" : "no",
                    contentFlags & Layer::CONTENT_NATIVE_VR ? "YES" : "no",
                    contentFlags & Layer::CONTENT_OPAQUE ? "YES" : "no");
#endif

      if (layer->GetEffectiveVisibleRegion().IsEmpty() &&
          !layer->AsContainerLayer()) {
        continue;
      }

      // XXX this is not the right calculation I don't think.  The manager world transform
      // should not be relevant here if we're going to apply a VR distortion.
      nsIntRect clipRect = layer->
        CalculateScissorRect(eyeClipRect, &aManager->GetWorldTransform());
      if (clipRect.IsEmpty()) {
        continue;
      }

      layerToRender->Prepare(clipRect);

      if (!(contentFlags & Layer::CONTENT_PRESERVE_3D) ||
          contentFlags & Layer::CONTENT_NATIVE_VR)
      {
        continue;
      }

      layerToRender->RenderLayer(clipRect);

      if (gfxPrefs::LayersScrollGraph()) DrawVelGraph(clipRect, aManager, layer);
      if (gfxPrefs::UniformityInfo()) PrintUniformityInfo(layer);
      if (gfxPrefs::DrawLayerInfo()) DrawLayerInfo(clipRect, aManager, layer);
    }
  }

  // restore previous global transform
  aContainer->mEffectiveTransform = origTransform;
  aContainer->ComputeEffectiveTransformsForChildren(origTransform);

  // Unbind the current surface and rebind the previous one.
#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    RefPtr<gfx::DataSourceSurface> surf = surface->Dump(aManager->GetCompositor());
    if (surf) {
      WriteSnapshotToDumpFile(aContainer, surf);
    }
  }
#endif

  compositor->PopRenderTarget();

  gfx::Rect rect(visibleRect.x, visibleRect.y, visibleRect.width * 2, visibleRect.height);
  gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width * 2, aClipRect.height);

  // The VR geometry may not cover the entire area; we need to fill with a solid color
  // first.
  // XXX should DrawQuad handle this on its own?  Is there a time where we wouldn't want
  // to do this? (e.g. something like Cardboard would not require distortion so will fill
  // the entire rect)
#if 0
  EffectChain solidEffect(aContainer);
  solidEffect.mPrimaryEffect = new EffectSolidColor(Color(0.0, 0.0, 0.0, 1.0));
  aManager->GetCompositor()->DrawQuad(rect, clipRect, solidEffect, opacity,
                                      aContainer->GetEffectiveTransform());
#endif

  // draw the temporary surface with VR distortion to the original destination
  EffectChain vrEffect(aContainer);
  vrEffect.mPrimaryEffect = new EffectVRDistortion(aHMD, surface);
  //vrEffect.mPrimaryEffect = new EffectRenderTarget(surface);

  // XXX we shouldn't use visibleRect here -- the VR distortion needs to know the
  // full rect, not just the visible one.  Luckily, right now, VR distortion is only
  // rendered when the element is fullscreen, so the visibleRect will be right anyway.
  aManager->GetCompositor()->DrawQuad(rect, clipRect, vrEffect, opacity,
                                      aContainer->GetEffectiveTransform());
}

/* all of the prepared data that we need in RenderLayer() */
struct PreparedData
{
  RefPtr<CompositingRenderTarget> mTmpTarget;
  nsAutoTArray<PreparedLayer, 12> mLayers;
};

// ContainerPrepare is shared between RefLayer and ContainerLayer
template<class ContainerT> void
ContainerPrepare(ContainerT* aContainer,
                 LayerManagerComposite* aManager,
                 const nsIntRect& aClipRect)
{
  aContainer->mPrepared = MakeUnique<PreparedData>();

  gfx::VRHMDInfo *hmdInfo = aContainer->GetVRHMDInfo();
  if (hmdInfo && hmdInfo->GetConfiguration().IsValid()) {
    // we're not going to do anything here; instead, we'll do it all in ContainerRender.
    // XXX fix this; we can win with the same optimizations.  Specifically, we
    // want to render thebes layers only once and then composite the intermeidate surfaces
    // with different transforms twice.
    return;
  }

  //printf_stderr(">>> ContainerRender\n");

  /**
   * Determine which layers to draw.
   */
  nsAutoTArray<Layer*, 12> children;
  aContainer->SortChildrenBy3DZOrder(children);

  for (uint32_t i = 0; i < children.Length(); i++) {
    LayerComposite* layerToRender = static_cast<LayerComposite*>(children.ElementAt(i)->ImplData());

    if (layerToRender->GetLayer()->GetEffectiveVisibleRegion().IsEmpty() &&
        !layerToRender->GetLayer()->AsContainerLayer()) {
      continue;
    }

    nsIntRect clipRect = layerToRender->GetLayer()->
        CalculateScissorRect(aClipRect, &aManager->GetWorldTransform());
    if (clipRect.IsEmpty()) {
      continue;
    }

    nsIntRegion savedVisibleRegion;
    bool restoreVisibleRegion = false;
    if (i + 1 < children.Length() &&
        layerToRender->GetLayer()->GetEffectiveTransform().IsIdentity()) {
      LayerComposite* nextLayer = static_cast<LayerComposite*>(children.ElementAt(i + 1)->ImplData());
      nsIntRect nextLayerOpaqueRect;
      if (nextLayer && nextLayer->GetLayer()) {
        nextLayerOpaqueRect = GetOpaqueRect(nextLayer->GetLayer());
      }
      if (!nextLayerOpaqueRect.IsEmpty()) {
        savedVisibleRegion = layerToRender->GetShadowVisibleRegion();
        nsIntRegion visibleRegion;
        visibleRegion.Sub(savedVisibleRegion, nextLayerOpaqueRect);
        if (visibleRegion.IsEmpty()) {
          continue;
        }
        layerToRender->SetShadowVisibleRegion(visibleRegion);
        restoreVisibleRegion = true;
      }
    }
    layerToRender->Prepare(clipRect);
    aContainer->mPrepared->mLayers.AppendElement(PreparedLayer(layerToRender, clipRect, restoreVisibleRegion, savedVisibleRegion));
  }

  /**
   * Setup our temporary surface for rendering the contents of this container.
   */

  bool surfaceCopyNeeded;
  // DefaultComputeSupportsComponentAlphaChildren can mutate aContainer so call it unconditionally
  aContainer->DefaultComputeSupportsComponentAlphaChildren(&surfaceCopyNeeded);
  if (aContainer->UseIntermediateSurface()) {
    MOZ_PERFORMANCE_WARNING("gfx", "[%p] Container layer requires intermediate surface\n", aContainer);
    if (!surfaceCopyNeeded) {
      // If we don't need a copy we can render to the intermediate now to avoid
      // unecessary render target switching. This brings a big perf boost on mobile gpus.
      RefPtr<CompositingRenderTarget> surface = CreateTemporaryTarget(aContainer, aManager);
      RenderIntermediate(aContainer, aManager, aClipRect, surface);
      aContainer->mPrepared->mTmpTarget = surface;
    }
  }
}

template<class ContainerT> void
RenderLayers(ContainerT* aContainer,
	     LayerManagerComposite* aManager,
	     const nsIntRect& aClipRect)
{
  Compositor* compositor = aManager->GetCompositor();

  float opacity = aContainer->GetEffectiveOpacity();

  // If this is a scrollable container layer, and it's overscrolled, the layer's
  // contents are transformed in a way that would leave blank regions in the
  // composited area. If the layer has a background color, fill these areas
  // with the background color by drawing a rectangle of the background color
  // over the entire composited area before drawing the container contents.
  if (AsyncPanZoomController* apzc = aContainer->GetAsyncPanZoomController()) {
    // Make sure not to do this on a "scrollinfo" layer (one with an empty visible
    // region) because it's just a placeholder for APZ purposes.
    if (apzc->IsOverscrolled() && !aContainer->GetVisibleRegion().IsEmpty()) {
      gfxRGBA color = aContainer->GetBackgroundColor();
      // If the background is completely transparent, there's no point in
      // drawing anything for it. Hopefully the layers behind, if any, will
      // provide suitable content for the overscroll effect.
      if (color.a != 0.0) {
        EffectChain effectChain(aContainer);
        effectChain.mPrimaryEffect = new EffectSolidColor(ToColor(color));
        gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
        Compositor* compositor = aManager->GetCompositor();
        compositor->DrawQuad(compositor->ClipRectInLayersCoordinates(clipRect),
            clipRect, effectChain, opacity, Matrix4x4());
      }
    }
  }

  for (size_t i = 0u; i < aContainer->mPrepared->mLayers.Length(); i++) {
    PreparedLayer& preparedData = aContainer->mPrepared->mLayers[i];
    LayerComposite* layerToRender = preparedData.mLayer;
    nsIntRect clipRect = preparedData.mClipRect;
    if (layerToRender->HasLayerBeenComposited()) {
      // Composer2D will compose this layer so skip GPU composition
      // this time & reset composition flag for next composition phase
      layerToRender->SetLayerComposited(false);
      nsIntRect clearRect = layerToRender->GetClearRect();
      if (!clearRect.IsEmpty()) {
        // Clear layer's visible rect on FrameBuffer with transparent pixels
        gfx::Rect fbRect(clearRect.x, clearRect.y, clearRect.width, clearRect.height);
        compositor->ClearRect(fbRect);
        layerToRender->SetClearRect(nsIntRect(0, 0, 0, 0));
      }
    } else {
      layerToRender->RenderLayer(clipRect);
    }

    if (preparedData.mRestoreVisibleRegion) {
      // Restore the region in case it's not covered by opaque content next time
      layerToRender->SetShadowVisibleRegion(preparedData.mSavedVisibleRegion);
    }

    if (gfxPrefs::LayersScrollGraph()) {
      DrawVelGraph(clipRect, aManager, layerToRender->GetLayer());
    }

    if (gfxPrefs::UniformityInfo()) {
      PrintUniformityInfo(layerToRender->GetLayer());
    }

    if (gfxPrefs::DrawLayerInfo()) {
      DrawLayerInfo(clipRect, aManager, layerToRender->GetLayer());
    }
    // invariant: our GL context should be current here, I don't think we can
    // assert it though
  }
}

template<class ContainerT> RefPtr<CompositingRenderTarget>
CreateTemporaryTarget(ContainerT* aContainer,
                      LayerManagerComposite* aManager)
{
  Compositor* compositor = aManager->GetCompositor();
  nsIntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();
  SurfaceInitMode mode = INIT_MODE_CLEAR;
  gfx::IntRect surfaceRect = gfx::IntRect(visibleRect.x, visibleRect.y,
                                          visibleRect.width, visibleRect.height);
  if (aContainer->GetEffectiveVisibleRegion().GetNumRects() == 1 &&
      (aContainer->GetContentFlags() & Layer::CONTENT_OPAQUE))
  {
    mode = INIT_MODE_NONE;
  }
  return compositor->CreateRenderTarget(surfaceRect, mode);
}

template<class ContainerT> RefPtr<CompositingRenderTarget>
CreateTemporaryTargetAndCopyFromBackground(ContainerT* aContainer,
                                           LayerManagerComposite* aManager)
{
  Compositor* compositor = aManager->GetCompositor();
  nsIntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();
  RefPtr<CompositingRenderTarget> previousTarget = compositor->GetCurrentRenderTarget();
  gfx::IntRect surfaceRect = gfx::IntRect(visibleRect.x, visibleRect.y,
                                          visibleRect.width, visibleRect.height);

  gfx::IntPoint sourcePoint = gfx::IntPoint(visibleRect.x, visibleRect.y);

  gfx::Matrix4x4 transform = aContainer->GetEffectiveTransform();
  DebugOnly<gfx::Matrix> transform2d;
  MOZ_ASSERT(transform.Is2D(&transform2d) && !gfx::ThebesMatrix(transform2d).HasNonIntegerTranslation());
  sourcePoint += gfx::IntPoint(transform._41, transform._42);

  sourcePoint -= compositor->GetCurrentRenderTarget()->GetOrigin();

  return compositor->CreateRenderTargetFromSource(surfaceRect, previousTarget, sourcePoint);
}

template<class ContainerT> void
RenderIntermediate(ContainerT* aContainer,
                   LayerManagerComposite* aManager,
                   const nsIntRect& aClipRect,
                   RefPtr<CompositingRenderTarget> surface)
{
  Compositor* compositor = aManager->GetCompositor();
  RefPtr<CompositingRenderTarget> previousTarget = compositor->GetCurrentRenderTarget();

  if (!surface) {
    return;
  }

  compositor->SetRenderTarget(surface);
  // pre-render all of the layers into our temporary
  RenderLayers(aContainer, aManager, aClipRect);
  // Unbind the current surface and rebind the previous one.
  compositor->SetRenderTarget(previousTarget);
}

template<class ContainerT> void
ContainerRender(ContainerT* aContainer,
                 LayerManagerComposite* aManager,
                 const nsIntRect& aClipRect)
{
  MOZ_ASSERT(aContainer->mPrepared);

  gfx::vr::HMDInfo *hmdInfo = aContainer->GetVRHMDInfo();
  if (hmdInfo && hmdInfo->GetConfiguration().IsValid()) {
    ContainerRenderVR(aContainer, aManager, aClipRect, hmdInfo);
    aContainer->mPrepared = nullptr;
    return;
  }

  if (aContainer->UseIntermediateSurface()) {
    RefPtr<CompositingRenderTarget> surface;

    if (!aContainer->mPrepared->mTmpTarget) {
      // we needed to copy the background so we waited until now to render the intermediate
      surface = CreateTemporaryTargetAndCopyFromBackground(aContainer, aManager);
      RenderIntermediate(aContainer, aManager, aClipRect, surface);
    } else {
      surface = aContainer->mPrepared->mTmpTarget;
    }

    float opacity = aContainer->GetEffectiveOpacity();

    nsIntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();
#ifdef MOZ_DUMP_PAINTING
    if (gfxUtils::sDumpPainting) {
      RefPtr<gfx::DataSourceSurface> surf = surface->Dump(aManager->GetCompositor());
      if (surf) {
        WriteSnapshotToDumpFile(aContainer, surf);
      }
    }
#endif

    EffectChain effectChain(aContainer);
    LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(aContainer->GetMaskLayer(),
                                                            effectChain,
                                                            !aContainer->GetTransform().CanDraw2D());
    if (autoMaskEffect.Failed()) {
      NS_WARNING("Failed to apply a mask effect.");
      return;
    }

    aContainer->AddBlendModeEffect(effectChain);
    effectChain.mPrimaryEffect = new EffectRenderTarget(surface);

    gfx::Rect rect(visibleRect.x, visibleRect.y, visibleRect.width, visibleRect.height);
    gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
    aManager->GetCompositor()->DrawQuad(rect, clipRect, effectChain, opacity,
                                        aContainer->GetEffectiveTransform());
  } else {
    RenderLayers(aContainer, aManager, aClipRect);
  }
  aContainer->mPrepared = nullptr;

  if (aContainer->GetFrameMetrics().IsScrollable()) {
    const FrameMetrics& frame = aContainer->GetFrameMetrics();
    LayerRect layerBounds = frame.mCompositionBounds * ParentLayerToLayerScale(1.0);
    gfx::Rect rect(layerBounds.x, layerBounds.y, layerBounds.width, layerBounds.height);
    gfx::Rect clipRect(aClipRect.x, aClipRect.y, aClipRect.width, aClipRect.height);
    aManager->GetCompositor()->DrawDiagnostics(DiagnosticFlags::CONTAINER,
                                               rect, clipRect,
                                               aContainer->GetEffectiveTransform());
  }
}

ContainerLayerComposite::ContainerLayerComposite(LayerManagerComposite *aManager)
  : ContainerLayer(aManager, nullptr)
  , LayerComposite(aManager)
{
  MOZ_COUNT_CTOR(ContainerLayerComposite);
  mImplData = static_cast<LayerComposite*>(this);
}

ContainerLayerComposite::~ContainerLayerComposite()
{
  MOZ_COUNT_DTOR(ContainerLayerComposite);

  // We don't Destroy() on destruction here because this destructor
  // can be called after remote content has crashed, and it may not be
  // safe to free the IPC resources of our children.  Those resources
  // are automatically cleaned up by IPDL-generated code.
  //
  // In the common case of normal shutdown, either
  // LayerManagerComposite::Destroy(), a parent
  // *ContainerLayerComposite::Destroy(), or Disconnect() will trigger
  // cleanup of our resources.
  while (mFirstChild) {
    RemoveChild(mFirstChild);
  }
}

void
ContainerLayerComposite::Destroy()
{
  if (!mDestroyed) {
    while (mFirstChild) {
      static_cast<LayerComposite*>(GetFirstChild()->ImplData())->Destroy();
      RemoveChild(mFirstChild);
    }
    mDestroyed = true;
  }
}

LayerComposite*
ContainerLayerComposite::GetFirstChildComposite()
{
  if (!mFirstChild) {
    return nullptr;
   }
  return static_cast<LayerComposite*>(mFirstChild->ImplData());
}

void
ContainerLayerComposite::RenderLayer(const nsIntRect& aClipRect)
{
  ContainerRender(this, mCompositeManager, aClipRect);
}

void
ContainerLayerComposite::Prepare(const nsIntRect& aClipRect)
{
  ContainerPrepare(this, mCompositeManager, aClipRect);
}

void
ContainerLayerComposite::CleanupResources()
{
  for (Layer* l = GetFirstChild(); l; l = l->GetNextSibling()) {
    LayerComposite* layerToCleanup = static_cast<LayerComposite*>(l->ImplData());
    layerToCleanup->CleanupResources();
  }
}

RefLayerComposite::RefLayerComposite(LayerManagerComposite* aManager)
  : RefLayer(aManager, nullptr)
  , LayerComposite(aManager)
{
  mImplData = static_cast<LayerComposite*>(this);
}

RefLayerComposite::~RefLayerComposite()
{
  Destroy();
}

void
RefLayerComposite::Destroy()
{
  MOZ_ASSERT(!mFirstChild);
  mDestroyed = true;
}

LayerComposite*
RefLayerComposite::GetFirstChildComposite()
{
  if (!mFirstChild) {
    return nullptr;
   }
  return static_cast<LayerComposite*>(mFirstChild->ImplData());
}

void
RefLayerComposite::RenderLayer(const nsIntRect& aClipRect)
{
  ContainerRender(this, mCompositeManager, aClipRect);
}

void
RefLayerComposite::Prepare(const nsIntRect& aClipRect)
{
  ContainerPrepare(this, mCompositeManager, aClipRect);
}


void
RefLayerComposite::CleanupResources()
{
}

} /* layers */
} /* mozilla */
