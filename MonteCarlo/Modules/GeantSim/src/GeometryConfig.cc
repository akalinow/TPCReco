/**
 * @file GeometryConfig.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of GeometryConfig class
 */

#include <GeometryConfig.hh>
#include <CentralConfig.hh>
#include "TPCReco/ConfigManager.h"
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
    // NOTE: Arithmetic BOOST ptree members are accessed via static method: ConfigManager::getScalar<double>(tree, "some.branch")
    //       instead of: tree.get<double>("some.branch")
    //       in order to enable MATH expressions in MC JSON config files (e.g. "M_PI/2")

    central_config = CentralConfig::GetInstance();
    geometryNode = central_config->GetNode("GeometryConfig");
    // path_to_stl = geometryNode.get<std::string>("ModelPath"); // without MATH expressions
    path_to_stl = ConfigManager::getScalar<std::string>(geometryNode, "ModelPath");
    ParseMaterialColors();
    ParseGeometry();
}

void GeometryConfig::ParseGeometry() {

    for (const auto &mat: geometryNode.get_child("Solids")) {
        auto matName = mat.first;
        for (auto p: mat.second) {
            auto pattern = path_to_stl + std::string(p.second.data());
            glob_t glob_result;
            glob(pattern.c_str(), GLOB_TILDE, nullptr, &glob_result);// find stl files matching pattern
            for (unsigned int i = 0; i < glob_result.gl_pathc; i++) {
                SolidDescriptor solDesc;
                solDesc.name = std::string(glob_result.gl_pathv[i]);
                solDesc.filename = glob_result.gl_pathv[i];
                solDesc.material = matName;
                solDesc.color = material_colors[matName];
                solids.push_back(solDesc);
            }
        }
    }
}

void GeometryConfig::ParseMaterialColors() {
    // NOTE: Arithmetic BOOST ptree members are accessed via static method: ConfigManager::getScalar<double>(tree, "some.branch")
    //       instead of: tree.get<double>("some.branch")
    //       in order to enable MATH expressions in MC JSON config files (e.g. "M_PI/2")

    auto materials = geometryNode.get_child("MaterialColors");
    for (const auto &child: materials) {
        std::string mat_name = child.first;
        // auto red = child.second.get<int>("r"); // without MATH expressions
        // auto green = child.second.get<int>("g"); // without MATH expressions
        // auto blue = child.second.get<int>("b"); // without MATH expressions
        // auto alpha = child.second.get<float>("alpha"); // without MATH expressions
        auto red = ConfigManager::getScalar<int>(child.second, "r");
        auto green = ConfigManager::getScalar<int>(child.second, "g");
        auto blue = ConfigManager::getScalar<int>(child.second, "b");
        auto alpha = ConfigManager::getScalar<int>(child.second, "alpha");
        material_colors[mat_name] = G4Color(red / 255., green / 255., blue / 255., alpha);
    }
}


std::vector<SolidDescriptor> GeometryConfig::GetSolids() {
    return solids;
}
