#include <iostream>

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkExtractImageFilter.h"
#include "itkImageToVTKImageFilter.h"

#include <vtkSmartPointer.h>
#include <vtkImageViewer2.h>
#include <vtkRenderWindowInteractor.h>

int main(int argc, char *argv[])
{
    const char * filename;
    if (argc < 2)
    {
        filename = "/home/bulle/epita/s8/veiny_cuda /1-200/91.img.nii.gz"; // CHANGE DIRECTORY HERE
    }
    else
        filename = argv[1];

    // ITK types
    constexpr unsigned int Dimension3D = 3;
    using PixelType = float;
    using ImageType3D = itk::Image<PixelType, Dimension3D>;

    // Reader
    auto reader = itk::ImageFileReader<ImageType3D>::New();
    reader->SetFileName(filename);

    try
    {
        reader->Update();
    }
    catch (itk::ExceptionObject & err)
    {
        std::cerr << "Error reading image: " << err << std::endl;
        return EXIT_FAILURE;
    }

    ImageType3D::Pointer image3D = reader->GetOutput();
    ImageType3D::RegionType region = image3D->GetLargestPossibleRegion();
    auto size = region.GetSize();

    std::cout << "Image size: " << size[0] << " x " << size[1] << " x " << size[2] << std::endl;

    // Extract a 2D slice at the middle of z-axis
    constexpr unsigned int Dimension2D = 2;
    using ImageType2D = itk::Image<PixelType, Dimension2D>;

    using ExtractFilterType = itk::ExtractImageFilter<ImageType3D, ImageType2D>;
    auto extractFilter = ExtractFilterType::New();

    // Define extraction region (one slice)
    ImageType3D::IndexType start = region.GetIndex();
    ImageType3D::SizeType size2D = size;
    size2D[2] = 0; // extract a single slice in z-dim
    start[2] = size[2] / 2; // middle slice

    ImageType3D::RegionType extractRegion;
    extractRegion.SetIndex(start);
    extractRegion.SetSize(size2D);

    extractFilter->SetExtractionRegion(extractRegion);
    extractFilter->SetInput(image3D);
    extractFilter->SetDirectionCollapseToSubmatrix();

    try
    {
        extractFilter->Update();
    }
    catch (itk::ExceptionObject & err)
    {
        std::cerr << "Error extracting slice: " << err << std::endl;
        return EXIT_FAILURE;
    }

    // Convert to VTK image
    using ConnectorType = itk::ImageToVTKImageFilter<ImageType2D>;
    auto connector = ConnectorType::New();
    connector->SetInput(extractFilter->GetOutput());

    try
    {
        connector->Update();
    }
    catch (itk::ExceptionObject & err)
    {
        std::cerr << "Error converting to VTK image: " << err << std::endl;
        return EXIT_FAILURE;
    }

    // Visualize with VTK
    auto viewer = vtkSmartPointer<vtkImageViewer2>::New();
    viewer->SetInputData(connector->GetOutput());
    viewer->SetColorLevel(128);
    viewer->SetColorWindow(256);

    auto renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    viewer->SetupInteractor(renderWindowInteractor);
    viewer->Render();
    //viewer->GetRenderer()->ResetCamera();
    //viewer->SetWindowName("Middle Slice");
    renderWindowInteractor->Start();

    return EXIT_SUCCESS;
}
