#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageToVTKImageFilter.h"

#include <vtkSmartPointer.h>
#include <vtkImageViewer2.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCommand.h>
#include <vtkSphereSource.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>

struct FiducialPoint {
    double x, y, z;
};

std::vector<FiducialPoint> ReadFiducialsFromFCSV(const std::string& filepath) {
    std::ifstream file(filepath);
    std::string line;
    std::vector<FiducialPoint> points;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string item;
        std::vector<std::string> tokens;

        while (std::getline(ss, item, ',')) {
            tokens.push_back(item);
        }

        if (tokens.size() >= 4) {
            FiducialPoint p;
            p.x = std::stod(tokens[1]);
            p.y = std::stod(tokens[2]);
            p.z = std::stod(tokens[3]);
            points.push_back(p);
        }
    }

    return points;
}

class SliceScrollInteractorStyle : public vtkCommand
{
public:
    static SliceScrollInteractorStyle* New()
    {
        return new SliceScrollInteractorStyle;
    }

    void SetViewer(vtkImageViewer2* viewer) { this->Viewer = viewer; }
    void SetMinSlice(int minSlice) { this->MinSlice = minSlice; }
    void SetMaxSlice(int maxSlice) { this->MaxSlice = maxSlice; }
    void SetSpacing(double zSpacing) { this->ZSpacing = zSpacing; }
    void SetOrigin(double zOrigin) { this->ZOrigin = zOrigin; }

    void SetFiducialPoints(const std::vector<FiducialPoint>& points, vtkRenderer* renderer)
    {
        this->Fiducials = points;
        this->Renderer = renderer;
    }

    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
    {
        if (eventId != vtkCommand::KeyPressEvent)
            return;

        vtkRenderWindowInteractor* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
        std::string key = interactor->GetKeySym();

        int slice = Viewer->GetSlice();
        bool updated = false;

        if (key == "Up" || key == "Right")
        {
            if (slice < MaxSlice) {
                Viewer->SetSlice(slice + 1);
                updated = true;
            }
        }
        else if (key == "Down" || key == "Left")
        {
            if (slice > MinSlice) {
                Viewer->SetSlice(slice - 1);
                updated = true;
            }
        }

        if (updated) {
            std::cout << "Slice: " << Viewer->GetSlice() << std::endl;
            UpdateFiducialActors();
            Viewer->Render();
        }
    }

    void InitializeFiducialActors()
    {
        for (const auto& point : Fiducials) {
            auto sphereSource = vtkSmartPointer<vtkSphereSource>::New();
            sphereSource->SetCenter(point.x, point.y, point.z);
            sphereSource->SetRadius(1.0);

            auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(sphereSource->GetOutputPort());

            auto actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->SetColor(1.0, 0.0, 0.0); // red
            actor->SetVisibility(false); // initially hidden

            Renderer->AddActor(actor);
            FiducialActors.push_back(actor);
        }

        UpdateFiducialActors();
    }

private:
    void UpdateFiducialActors()
    {
        int currentSlice = Viewer->GetSlice();
        double sliceZ = currentSlice * ZSpacing + ZOrigin;
        double tolerance = ZSpacing / 2.0;

        for (size_t i = 0; i < Fiducials.size(); ++i) {
            double dz = std::abs(Fiducials[i].z - sliceZ);
            FiducialActors[i]->SetVisibility(dz < tolerance);
        }
    }

    vtkImageViewer2* Viewer = nullptr;
    int MinSlice = 0;
    int MaxSlice = 0;
    double ZSpacing = 1.0;
    double ZOrigin = 0.0;

    std::vector<FiducialPoint> Fiducials;
    std::vector<vtkSmartPointer<vtkActor>> FiducialActors;
    vtkRenderer* Renderer = nullptr;
};

int main(int argc, char* argv[])
{
    const char* imagePath = (argc > 1) ? argv[1] : "/home/bulle/epita/s8/veiny_cuda/1-200/91.img.nii.gz";
    std::string fcsvPath = "/home/bulle/epita/s8/veiny_cuda/CenterlineCurve.fcsv"; // CHANGE PATH

    // ITK image type
    constexpr unsigned int Dimension = 3;
    using PixelType = float;
    using ImageType = itk::Image<PixelType, Dimension>;

    // Read image
    auto reader = itk::ImageFileReader<ImageType>::New();
    reader->SetFileName(imagePath);

    try {
        reader->Update();
    } catch (itk::ExceptionObject& err) {
        std::cerr << "Error reading image: " << err << std::endl;
        return EXIT_FAILURE;
    }

    ImageType::Pointer image = reader->GetOutput();
    auto region = image->GetLargestPossibleRegion();
    auto size = region.GetSize();
    auto spacing = image->GetSpacing();
    auto origin = image->GetOrigin();

    std::cout << "Image size: " << size[0] << " x " << size[1] << " x " << size[2] << std::endl;

    // Convert ITK -> VTK
    using ConnectorType = itk::ImageToVTKImageFilter<ImageType>;
    auto connector = ConnectorType::New();
    connector->SetInput(image);
    connector->Update();

    // VTK viewer
    auto viewer = vtkSmartPointer<vtkImageViewer2>::New();
    viewer->SetInputData(connector->GetOutput());
    viewer->SetSliceOrientationToXY();
    viewer->SetColorLevel(128);
    viewer->SetColorWindow(256);
    viewer->SetSlice(0);

    auto renderer = viewer->GetRenderer(); // use viewer's internal renderer
    viewer->GetRenderWindow()->Render();

    // Read fiducials
    auto fiducials = ReadFiducialsFromFCSV(fcsvPath);

    // Interactor
    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    viewer->SetupInteractor(interactor);

    auto style = vtkSmartPointer<SliceScrollInteractorStyle>::New();
    style->SetViewer(viewer);
    style->SetMinSlice(0);
    style->SetMaxSlice(size[2] - 1);
    style->SetSpacing(spacing[2]);
    style->SetOrigin(origin[2]);
    style->SetFiducialPoints(fiducials, renderer);
    style->InitializeFiducialActors();

    interactor->AddObserver(vtkCommand::KeyPressEvent, style);

    interactor->Initialize();
    interactor->Start();

    return EXIT_SUCCESS;
}
