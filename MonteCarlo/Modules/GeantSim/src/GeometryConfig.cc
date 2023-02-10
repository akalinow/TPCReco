/**
 * @file GeometryConfig.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of GeometryConfig class
 */

#include <GeometryConfig.hh>
#include "G4SystemOfUnits.hh"
#include "globals.hh"
/// \cond
#include <glob.h>
/// \endcond




GeometryConfig *GeometryConfig::instance = nullptr;

GeometryConfig *GeometryConfig::GetInstance() {
    if (instance == nullptr)
        instance = new GeometryConfig();
    return instance;
}

GeometryConfig::GeometryConfig() {
    central_config = CentralConfig::GetInstance();
    config_name = central_config->Get("geometry_config_path");
    result = config.load_file(config_name.c_str());
    if (!result)
        std::cerr
                << "\e[31mError opening geometry config file. Check if file exists, and that the formatting is correct!!\e[0m"
                << std::endl;
    path_to_stl = config.child("model_path").text().as_string();
    ParseMaterialColors();
    ParseGeometry();
}

void GeometryConfig::ParseGeometry() {
    pugi::xml_node pugi_solids = config.child("solids");
    for (auto sol: pugi_solids) {

        glob_t glob_result;
        std::string pattern = path_to_stl + sol.text().as_string();
        std::string material_name = sol.attribute("material").value();
        std::string solid_name = sol.name();
        int instanceID = 0;
        glob(pattern.c_str(), GLOB_TILDE, nullptr, &glob_result);// find stl files matching pattern
        for (unsigned int i = 0; i < glob_result.gl_pathc; i++) {
            SolidDescriptor solDesc;
            solDesc.name = solid_name + "_" + std::to_string(instanceID++);
            solDesc.filename = glob_result.gl_pathv[i];
            solDesc.material = material_name;
            solDesc.color = material_colors[material_name];
            solids.push_back(solDesc);
        }

    }
}

void GeometryConfig::ParseMaterialColors() {
    pugi::xml_node materials = config.child("materials");
    for (auto child: materials.children()) {
        std::string mat_name = child.name();
        int red = child.child("r").text().as_int();
        int green = child.child("g").text().as_int();
        int blue = child.child("b").text().as_int();
        float alpha = child.child("alpha").text().as_float();
        material_colors[mat_name] = G4Color(red / 255., green / 255., blue / 255., alpha);
    }
}


std::vector<SolidDescriptor> GeometryConfig::GetSolids() {
    return solids;
}
