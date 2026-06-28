#include "Project.h"
#include <stdexcept>

Path& Project::addPath(const std::string& name) {
    std::string actualName = name.empty()
        ? "Path " + std::to_string(++m_pathCounter)
        : name;
    paths.emplace_back(std::move(actualName));
    markModified();
    return paths.back();
}

void Project::removePath(int index) {
    if (index < 0 || index >= static_cast<int>(paths.size()))
        throw std::out_of_range("removePath: invalid index");
    paths.erase(paths.begin() + index);
    markModified();
}

void Project::mergePaths(int indexA, int indexB) {
    int n = static_cast<int>(paths.size());
    if (indexA < 0 || indexA >= n || indexB < 0 || indexB >= n || indexA == indexB)
        throw std::out_of_range("mergePaths: invalid indices");

    paths[indexA].merge(paths[indexB]);
    paths.erase(paths.begin() + indexB);
    markModified();
}
