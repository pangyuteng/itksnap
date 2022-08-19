#include "MeshDisplayMappingPolicy.h"
#include "IRISException.h"
#include "vtkScalarBarActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkLookupTable.h"
#include "Generic3DRenderer.h"
#include "vtkActor.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "Generic3DRenderer.h"
#include "Rebroadcaster.h"
#include "ActorPool.h"
#include "vtkProperty.h"

// ==================================================
//  MeshDisplayMappingPolicy Implementation
// ==================================================

MeshDisplayMappingPolicy::MeshDisplayMappingPolicy()
{

}

IntensityCurveInterface *
MeshDisplayMappingPolicy::GetIntensityCurve() const
{
  return m_IntensityCurve;
}

ColorMap *
MeshDisplayMappingPolicy::GetColorMap() const
{
  return m_ColorMap;
}

ImageWrapperBase::DisplaySlicePointer
MeshDisplayMappingPolicy::GetDisplaySlice(unsigned int)
{
  return nullptr;
}

void
MeshDisplayMappingPolicy::Save(Registry &)
{

}

void
MeshDisplayMappingPolicy::Restore(Registry &)
{

}

Vector2d
MeshDisplayMappingPolicy::GetNativeImageRangeForCurve()
{
  // Get Active prop
  auto prop = m_Wrapper->GetActiveDataArrayProperty();

  return Vector2d(prop->GetMin(), prop->GetMax());
}

ScalarImageHistogram *
MeshDisplayMappingPolicy::GetHistogram(int nBins)
{
  return m_Wrapper->GetActiveDataArrayProperty()->GetHistogram(nBins);
}

void
MeshDisplayMappingPolicy::SetColorMap(ColorMap *map)
{
  m_ColorMap = map;
	m_Wrapper->InvokeEvent(WrapperDisplayMappingChangeEvent());
}

void
MeshDisplayMappingPolicy::SetIntensityCurve(IntensityCurveVTK *curve)
{
  m_IntensityCurve = curve;
	m_Wrapper->InvokeEvent(WrapperDisplayMappingChangeEvent());
}

void
MeshDisplayMappingPolicy::
UpdateActorMap(ActorPool *pool, unsigned int timepoint)
{
  auto meshes = m_Wrapper->GetMeshAssembly(timepoint);

  if (!meshes)
    return;

  auto actorMap = pool->GetActorMap();

  // Now create actors for all the meshes that don't have them yet
  // and update actors if the mesh is updated
  for(auto it_mesh = meshes->cbegin(); it_mesh != meshes->cend(); ++it_mesh)
    {
    // Pop a spare actor or create a new actor
    vtkActor *actor = pool->GetNewActor();

    // connect the rendering pipeline for the mesh
    auto mapper = static_cast<vtkPolyDataMapper*>(actor->GetMapper());
    mapper->SetInputData(it_mesh->second->GetPolyData());
    actor->SetMapper(mapper);

    // Keep the actor in the map
    actorMap->insert(std::make_pair(it_mesh->first, actor));
    }// end of updating actors
}



void
MeshDisplayMappingPolicy::SetMesh(MeshWrapperBase *mesh_wrapper)
{
  // Set Wrapper
  m_Wrapper = mesh_wrapper;

  // Lookup Table
  m_LookupTable = vtkLookupTable::New();

  // Color Map
  auto cMap = ColorMap::New();
  cMap->SetToSystemPreset(ColorMap::SystemPreset::COLORMAP_WINTER);
  this->SetColorMap(cMap);

  // Intensity Curve
  auto curve = IntensityCurveVTK::New();
  curve->Initialize();
  this->SetIntensityCurve(curve);

  m_Initialized = true;
}

vtkLookupTable*
MeshDisplayMappingPolicy::
GetLookupTable()
{
  UpdateLUT();
  return m_LookupTable;
}

MeshWrapperBase *
MeshDisplayMappingPolicy::GetMeshLayer()
{
  return m_Wrapper;
}

// ==================================================
//  GenericMeshDisplayMappingPolicy Implementation
// ==================================================
GenericMeshDisplayMappingPolicy::
GenericMeshDisplayMappingPolicy()
{
	m_UpdateCallbackCmd = UpdateGenericMeshDMPCommand::New();
	m_UpdateCallbackCmd->SetDMP(this);
}

GenericMeshDisplayMappingPolicy::
~GenericMeshDisplayMappingPolicy()
{

}

void
GenericMeshDisplayMappingPolicy::
UpdateAppearance(ActorPool *pool, unsigned int)
{
  // Get active data array property
  auto prop = m_Wrapper->GetActiveDataArrayProperty();

  if (!prop)
    return;

  // This should never happen, check data property implementation
  assert(prop->GetType() != MeshDataType::COUNT);

  // Update lookup table
  UpdateLUT();

	// Build the actorMap
  auto actorMap = pool->GetActorMap();

  for (auto it = actorMap->begin(); it != actorMap->end(); ++it)
    {
    auto actor = it->second;
    auto mapper = static_cast<vtkPolyDataMapper*>(actor->GetMapper());

    // Configure mapper
    mapper->SetLookupTable(m_LookupTable);
    mapper->UseLookupTableScalarRangeOn();

    // -- point/cell data specific logic
    if (prop->GetType() == MeshDataType::POINT_DATA)
      {
      mapper->SetScalarModeToUsePointData();
      auto pointData = mapper->GetInput()->GetPointData();
      pointData->SetActiveAttribute(prop->GetName(),
                                    vtkDataSetAttributes::AttributeTypes::SCALARS);
      }
    else if (prop->GetType() == MeshDataType::CELL_DATA)
      {
      mapper->SetScalarModeToUseCellData();
      auto cellData = mapper->GetInput()->GetCellData();
      cellData->SetActiveAttribute(prop->GetName(),
                                   vtkDataSetAttributes::AttributeTypes::SCALARS);
      }

    // -- set active attribute
    mapper->SetColorModeToMapScalars();
		}
}

void GenericMeshDisplayMappingPolicy::
ConfigureLegend(vtkScalarBarActor* legend)
{
  legend->SetLookupTable(m_LookupTable);

  auto prop = m_Wrapper->GetActiveDataArrayProperty();

  if (!prop)
    return;

  legend->SetTitle(prop->GetName());
}


void
GenericMeshDisplayMappingPolicy::
UpdateLUT()
{
  // Get the active property
  auto prop = m_Wrapper->GetActiveDataArrayProperty();

  if (!prop)
		return;

	// Check vector mode setting for multi-component data
	using VectorMode = MeshLayerDataArrayProperty::VectorMode;

	vtkIdType activeComp = -1;

	if (prop->GetNumberOfComponents() > 1)
		{
		VectorMode activeVecMode = prop->GetActiveVectorMode();

		// Set which value lut will use for vector data
		switch (activeVecMode)
			{
			case VectorMode::MAGNITUDE:
				{
				m_LookupTable->SetVectorMode(vtkScalarsToColors::MAGNITUDE);
				this->SetIntensityCurve(prop->GetIntensityCurve());
				break;
				}
			default:
				{
				m_LookupTable->SetVectorMode(vtkScalarsToColors::COMPONENT);
				m_LookupTable->SetVectorComponent(prop->GetActiveComponentId());
				activeComp = prop->GetActiveComponentId();
				auto compCurve = prop->GetActiveComponent().m_IntensityCurve;
				}
			}
		}

	// Build the lookup table
	// Find the min/max ratio to value range based on contrast curve
	double dmin, dmax; // data min and max
	dmin = prop->GetMin(activeComp);
	dmax = prop->GetMax(activeComp);

	float rMin, tMin, rMax, tMax;
	m_IntensityCurve->GetControlPoint(0, rMin, tMin);
	m_IntensityCurve->GetControlPoint(m_IntensityCurve->GetControlPointCount() - 1, rMax, tMax);

	double lutMin, lutMax, drange;
	drange = dmax - dmin;
	lutMin = dmin + drange * rMin;
	lutMax = dmin + drange * rMax;

	m_LookupTable->SetRange(lutMin, lutMax); // lut range should be contrast range

	// Prepare color generation
	const size_t numClr = 256;
	double numdiv = 1.0/numClr;
  double clrdiv = 1.0/255.0;
	double indRange = rMax - rMin;

  m_LookupTable->SetNumberOfColors(numClr);
  for (auto i = 0u; i < numClr; ++i)
    {
		float ind = rMin + indRange * i * numdiv;
		auto val = m_IntensityCurve->Evaluate(ind);
		auto rgbaC = m_ColorMap->MapIndexToRGBA(val);

		double rgbaD[4] = {rgbaC[0] * clrdiv, rgbaC[1] * clrdiv,
											 rgbaC[2] * clrdiv, rgbaC[3] * clrdiv};

    m_LookupTable->SetTableValue(i, rgbaD);
    }

  m_LookupTable->Build();
}

void
GenericMeshDisplayMappingPolicy
::UpdateGenericMeshDMPCommand
::Execute(itk::Object *, const itk::EventObject &)
{
	std::cout << "[DMPUpdateCommand] execute()" << std::endl;
	// Just pull the latest active info
	auto layer_prop = m_DMP->GetMeshLayer()->GetActiveDataArrayProperty();
	m_DMP->SetColorMap(layer_prop->GetColorMap());
	m_DMP->SetIntensityCurve(layer_prop->GetIntensityCurve());
}

// ==================================================
//  LabelMeshDisplayMappingPolicy Implementation
// ==================================================
LabelMeshDisplayMappingPolicy::
LabelMeshDisplayMappingPolicy()
{

}

LabelMeshDisplayMappingPolicy::
~LabelMeshDisplayMappingPolicy()
{

}

void
LabelMeshDisplayMappingPolicy::
UpdateAppearance(ActorPool *pool, unsigned int)
{
  auto actorMap = pool->GetActorMap();

  // Always update LUT first
  UpdateLUT();

  for (auto it = actorMap->begin(); it != actorMap->end(); ++it)
    {
    LabelType label = it->first;
    auto actor = it->second;
    const ColorLabel &cl = m_ColorLabelTable->GetColorLabel(label);

    auto prop = actor->GetProperty();
    prop->SetColor(cl.GetRGBAsDoubleVector().data_block());
    prop->SetOpacity(cl.GetAlpha() / 255.0);
    }
}

void
LabelMeshDisplayMappingPolicy::
ConfigureLegend(vtkScalarBarActor* legend)
{
  legend->SetLookupTable(m_LookupTable);
}

void
LabelMeshDisplayMappingPolicy::
UpdateLUT()
{
	size_t numClr = m_ColorLabelTable->GetNumberOfValidLabels();
	m_LookupTable->SetIndexedLookup(true);
  m_LookupTable->SetNumberOfColors(numClr);
  m_LookupTable->SetRange(0, 6);

  double rgbaD[4];
  for (auto cit = m_ColorLabelTable->begin();
       cit != m_ColorLabelTable->end(); ++cit)
    {
    auto rgbD = cit->second.GetRGBAsDoubleVector().data_block();

    rgbaD[0] = rgbD[0];
    rgbaD[1] = rgbD[1];
    rgbaD[2] = rgbD[2];
    rgbaD[3] = cit->second.GetAlpha() / 255.0;

    m_LookupTable->SetTableValue(cit->first, rgbaD);
		m_LookupTable->SetAnnotation(cit->first, std::to_string(cit->first));
    }
  m_LookupTable->Build();
}


