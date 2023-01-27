#ifndef IMAGEWRAPPERTRAITS_H
#define IMAGEWRAPPERTRAITS_H

#include "SNAPCommon.h"
#include "ImageWrapperBase.h"
#include "RLEImageRegionIterator.h"

#include "CommonRepresentationPolicy.h"
#include "DisplayMappingPolicy.h"
#include "NativeIntensityMappingPolicy.h"

#include "ScalarImageWrapper.h"
#include "VectorImageWrapper.h"
#include "ColorMap.h"

namespace itk
{
  template <class TPixel, unsigned int VDim> class Image;
  template <class TPixel, unsigned int VDim> class VectorImage;
  template <class TPixel, unsigned int VDim> class VectorImageToImageAdaptor;
}

/*
class ScalarImageWrapperBase;
class VectorImageWrapperBase;
template <class TImage, class TBase> class VectorImageWrapper;
template <class TImage, class TBase> class ScalarImageWrapper;
*/

/**
 * Each of the traits classes below defines types and policies for a specific
 * type of image wrapper.
 */
class LabelImageWrapperTraits
{
public:
  typedef LabelImageWrapperTraits Self;

  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<LabelImageWrapperTraits> WrapperType;

  typedef LabelType ComponentType;
  typedef RLEImage<ComponentType> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;
  typedef RLEImage<ComponentType, 4> Image4DType;

  typedef IdentityInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef ColorLabelTableDisplayMappingPolicy<Self> DisplayMapping;
  typedef NullScalarImageWrapperCommonRepresentation<GreyType, Self> CommonRepresentationPolicy;

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, true);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, false);
};

class SpeedImageWrapperTraits
{
public:
  typedef SpeedImageWrapperTraits Self;

  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<SpeedImageWrapperTraits> WrapperType;

  typedef GreyType ComponentType;
  typedef itk::Image<ComponentType, 3> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;
  typedef itk::Image<ComponentType, 4> Image4DType;

  typedef SpeedImageInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef LinearColorMapDisplayMappingPolicy<Self> DisplayMapping;
  typedef NullScalarImageWrapperCommonRepresentation<GreyType, Self> CommonRepresentationPolicy;

  static void GetFixedIntensityRange(float &min, float &max)
    { min = -0x7fff; max = 0x7fff; }

  itkStaticConstMacro(DefaultColorMap, ColorMap::SystemPreset, ColorMap::COLORMAP_SPEED);

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, false);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, true);
};

class LevelSetImageWrapperTraits
{
public:
  typedef LevelSetImageWrapperTraits Self;

  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<LevelSetImageWrapperTraits> WrapperType;

  typedef float ComponentType;
  typedef itk::Image<ComponentType, 3> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;
  typedef itk::Image<ComponentType, 4> Image4DType;

  typedef IdentityInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef LinearColorMapDisplayMappingPolicy<Self> DisplayMapping;
  typedef NullScalarImageWrapperCommonRepresentation<GreyType, Self> CommonRepresentationPolicy;

  static void GetFixedIntensityRange(float &min, float &max)
    { min = -4.0; max = 4.0; }

  itkStaticConstMacro(DefaultColorMap, ColorMap::SystemPreset, ColorMap::COLORMAP_LEVELSET);

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, true);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, true);
};

/**
 * This helper traits class is used to select the appropriate common representation
 * policy depending on the internal pixel type
 */
template <class TPixel, class TWrapperTraits>
class DefaultCommonRepresentationPolicy
{
public:
  typedef CastingScalarImageWrapperCommonRepresentation<GreyType, TWrapperTraits> Policy;
};

template <class TWrapperTraits>
class DefaultCommonRepresentationPolicy<GreyType, TWrapperTraits>
{
public:
  typedef InPlaceScalarImageWrapperCommonRepresentation<GreyType, TWrapperTraits> Policy;
};

template <class TPixel, bool TLinearMapping>
class ComponentImageWrapperTraits
{
public:
  typedef ComponentImageWrapperTraits<TPixel, TLinearMapping> Self;

  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<Self> WrapperType;

  typedef TPixel ComponentType;
  typedef itk::VectorImageToImageAdaptor<ComponentType, 3> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;
  typedef itk::VectorImageToImageAdaptor<ComponentType, 4> Image4DType;

  // The type of internal to native intensity mapping used in the traits is controlled
  // by the boolean template parameter
  typedef typename std::conditional<TLinearMapping,
    LinearInternalToNativeIntensityMapping,
    IdentityInternalToNativeIntensityMapping>::type NativeIntensityMapping;

  typedef CachingCurveAndColorMapDisplayMappingPolicy<Self> DisplayMapping;
  typedef CastingScalarImageWrapperCommonRepresentation<GreyType, Self> CommonRepresentationPolicy;

  itkStaticConstMacro(DefaultColorMap, ColorMap::SystemPreset, ColorMap::COLORMAP_GREY);

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, false);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, false);
};

template <class TFunctor>
class VectorDerivedQuantityImageWrapperTraits
{
public:
  typedef VectorDerivedQuantityImageWrapperTraits<TFunctor> Self;
  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<Self> WrapperType;

  typedef typename TFunctor::OutputPixelType ComponentType;

  typedef typename TFunctor::InputPixelType InternalComponentType;
  typedef itk::VectorImage<InternalComponentType, 3> InternalImageType;
  typedef VectorToScalarImageAccessor<TFunctor> AccessorType;
  typedef itk::ImageAdaptor<InternalImageType, AccessorType> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;

  typedef itk::VectorImage<InternalComponentType, 4> InternalImage4DType;
  typedef itk::ImageAdaptor<InternalImage4DType, AccessorType> Image4DType;

  typedef IdentityInternalToNativeIntensityMapping NativeIntensityMapping;
  typedef CachingCurveAndColorMapDisplayMappingPolicy<Self> DisplayMapping;
  typedef CastingScalarImageWrapperCommonRepresentation<GreyType, Self> CommonRepresentationPolicy;

  itkStaticConstMacro(DefaultColorMap, ColorMap::SystemPreset, ColorMap::COLORMAP_GREY);

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, false);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, false);
};


template <class TPixel, bool TLinearMapping = false>
class AnatomicImageWrapperTraits
{
public:
  typedef AnatomicImageWrapperTraits<TPixel> Self;

  typedef VectorImageWrapperBase WrapperBaseType;
  typedef VectorImageWrapper<Self> WrapperType;

  // Component stuff
  typedef TPixel ComponentType;

  typedef ComponentImageWrapperTraits<ComponentType, TLinearMapping> ComponentWrapperTraits;
  typedef ScalarImageWrapper<ComponentWrapperTraits> ComponentWrapperType;

  typedef itk::VectorImage<ComponentType, 3> ImageType;
  typedef itk::VectorImage<ComponentType, 2> SliceType;
  typedef itk::VectorImage<ComponentType, 4> Image4DType;

  // The type of internal to native intensity mapping used in the traits is controlled
  // by the boolean template parameter
  typedef typename std::conditional<TLinearMapping,
    LinearInternalToNativeIntensityMapping,
    IdentityInternalToNativeIntensityMapping>::type NativeIntensityMapping;

  typedef MultiChannelDisplayMappingPolicy<Self> DisplayMapping;

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, false);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, false);
};


template <class TPixel, bool TLinearMapping = false>
class AnatomicScalarImageWrapperTraits
{
public:
  typedef AnatomicScalarImageWrapperTraits<TPixel> Self;

  typedef ScalarImageWrapperBase WrapperBaseType;
  typedef ScalarImageWrapper<Self> WrapperType;

  typedef TPixel ComponentType;
  typedef itk::Image<ComponentType, 3> ImageType;
  typedef itk::Image<ComponentType, 2> SliceType;
  typedef itk::Image<ComponentType, 4> Image4DType;

  typedef CachingCurveAndColorMapDisplayMappingPolicy<Self> DisplayMapping;
  typedef typename DefaultCommonRepresentationPolicy<TPixel, Self>::Policy CommonRepresentationPolicy;

  // The type of internal to native intensity mapping used in the traits is controlled
  // by the boolean template parameter
  typedef typename std::conditional<TLinearMapping,
    LinearInternalToNativeIntensityMapping,
    IdentityInternalToNativeIntensityMapping>::type NativeIntensityMapping;

  // Default color map
  itkStaticConstMacro(DefaultColorMap, ColorMap::SystemPreset, ColorMap::COLORMAP_GREY);

  // Whether this image is shown on top of all other layers by default
  itkStaticConstMacro(StickyByDefault, bool, false);

  // Whether this image is produced from another by a pipeline (e.g., speed image)
  itkStaticConstMacro(PipelineOutput, bool, false);
};

// This macro helps instantiate wrappers of different types in one go
#define AnatomicImageWrapperTraitsInstantiateMacro(x, y) \
  template class x< AnatomicImageWrapperTraits<unsigned char, y> >; \
  template class x< AnatomicImageWrapperTraits<char, y> >; \
  template class x< AnatomicImageWrapperTraits<unsigned short, y> >; \
  template class x< AnatomicImageWrapperTraits<short, y> >; \
  template class x< AnatomicImageWrapperTraits<float, y> >;

#define AnatomicScalarImageWrapperTraitsInstantiateMacro(x, y) \
  template class x< AnatomicScalarImageWrapperTraits<unsigned char, y> >; \
  template class x< AnatomicScalarImageWrapperTraits<char, y> >; \
  template class x< AnatomicScalarImageWrapperTraits<unsigned short, y> >; \
  template class x< AnatomicScalarImageWrapperTraits<short, y> >; \
  template class x< AnatomicScalarImageWrapperTraits<float, y> >;

#define ComponentImageWrapperTraitsInstantiateMacro(x, y) \
  template class x< ComponentImageWrapperTraits<unsigned char, y> >; \
  template class x< ComponentImageWrapperTraits<char, y> >; \
  template class x< ComponentImageWrapperTraits<unsigned short, y> >; \
  template class x< ComponentImageWrapperTraits<short, y> >; \
  template class x< ComponentImageWrapperTraits<float, y> >;




// Global typedefs for traits with GreyType
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMagnitudeFunctor>
  GreyVectorMagnitudeImageWrapperTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMaxFunctor>
  GreyVectorMaxImageWrapperTraits;
typedef VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMeanFunctor>
  GreyVectorMeanImageWrapperTraits;

// Some global typedefs
typedef SpeedImageWrapperTraits::WrapperType SpeedImageWrapper;
typedef LevelSetImageWrapperTraits::WrapperType LevelSetImageWrapper;


#endif // IMAGEWRAPPERTRAITS_H
