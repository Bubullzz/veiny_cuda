#include <iostream>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <itkImage.h>

int main() {
    // Simple VTK example
    vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->Update();

    // Simple ITK example
    constexpr unsigned int Dimension = 2;
    using PixelType = unsigned char;
    using ImageType = itk::Image<PixelType, Dimension>;
    ImageType::Pointer image = ImageType::New();
    std::cout << "hey" << std::endl;
    return 0;
}
