#include <iostream>

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkExtractImageFilter.h"
#include "itkImageToVTKImageFilter.h"

#include <vtkSmartPointer.h>
#include <vtkImageViewer2.h>
#include <vtkRenderWindowInteractor.h>

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

    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
    {
        if (eventId != vtkCommand::KeyPressEvent)
            return;

        vtkRenderWindowInteractor* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
        std::string key = interactor->GetKeySym();

        if (key == "Up" || key == "Right")
        {
            int slice = Viewer->GetSlice();
            if (slice < MaxSlice)
            {
                Viewer->SetSlice(slice + 1);
                Viewer->Render();
                std::cout << "Slice: " << slice + 1 << std::endl;
            }
        }
        else if (key == "Down" || key == "Left")
        {
            int slice = Viewer->GetSlice();
            if (slice > MinSlice)
            {
                Viewer->SetSlice(slice - 1);
                Viewer->Render();
                std::cout << "Slice: " << slice - 1 << std::endl;
            }
        }
    }

private:
    vtkImageViewer2* Viewer = nullptr;
    int MinSlice = 0;
    int MaxSlice = 0;
};


int main(int argc, char *argv[])
{
    const char * filename;
    if (argc < 2)
    {
        filename = "/home/bulle/epita/s8/veiny_cuda/1-200/91.img.nii.gz"; // CHANGE DIRECTORY HERE
    }
    else
        filename = argv[1];

    constexpr unsigned int Dimension = 3;
    using PixelType = float;
    using ImageType = itk::Image<PixelType, Dimension>;

    auto reader = itk::ImageFileReader<ImageType>::New();
    reader->SetFileName(filename);

    try
    {
        reader->Update();
    }
    catch (itk::ExceptionObject& err)
    {
        std::cerr << "Error reading image: " << err << std::endl;
        return EXIT_FAILURE;
    }

    ImageType::Pointer image = reader->GetOutput();
    ImageType::RegionType region = image->GetLargestPossibleRegion();
    auto size = region.GetSize();

    std::cout << "Image size: " << size[0] << " x " << size[1] << " x " << size[2] << std::endl;

    // We will display the whole 3D image with vtkImageViewer2 and scroll slices
    // Convert ITK image to VTK image
    using ConnectorType = itk::ImageToVTKImageFilter<ImageType>;
    auto connector = ConnectorType::New();
    connector->SetInput(image);
    try
    {
        connector->Update();
    }
    catch (itk::ExceptionObject& err)
    {
        std::cerr << "Error converting ITK to VTK: " << err << std::endl;
        return EXIT_FAILURE;
    }

    auto viewer = vtkSmartPointer<vtkImageViewer2>::New();
    viewer->SetInputData(connector->GetOutput());

    // Setup initial slice and slice range
    viewer->SetSlice(0);
    viewer->SetSliceOrientationToXY(); // axial slices
    viewer->SetColorLevel(128);
    viewer->SetColorWindow(256);
    viewer->Render();

    // Setup interactor with custom keypress event
    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    viewer->SetupInteractor(interactor);

    auto style = vtkSmartPointer<SliceScrollInteractorStyle>::New();
    style->SetViewer(viewer);
    style->SetMinSlice(0);
    style->SetMaxSlice(size[2] - 1);

    interactor->AddObserver(vtkCommand::KeyPressEvent, style);

    interactor->Initialize();
    interactor->Start();

    return EXIT_SUCCESS;
}