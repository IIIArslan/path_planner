#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QFrame>
#include <QList>
#include "core/Project.h"
#include "ui/FieldScene.h"

// Left-panel list of paths.
// Each row shows a colour dot, editable name, visibility toggle, and delete button.
// Stays in sync with the FieldScene via signals / rebuild().
class PathListPanel : public QWidget {
    Q_OBJECT
public:
    explicit PathListPanel(Project* project, FieldScene* scene, QWidget* parent = nullptr);

    // Switch to a different project (call before rebuild() on project replace).
    void setProject(Project* p) { m_project = p; }

    // Rebuild the entire list from the current project state.
    void rebuild();

    // Highlight the row at idx (−1 clears all highlights).
    void setSelectedRow(int idx);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void addRow(int pathIdx);
    void clearRows();
    void applyRowStyle(QFrame* row, bool selected);
    void openColorPicker(int pathIdx);
    void updateInfo(int idx);

    Project*    m_project;
    FieldScene* m_scene;

    QVBoxLayout* m_rowLayout  = nullptr;
    QList<QFrame*> m_rows;
    QLabel*      m_infoLabel  = nullptr;
    int          m_selectedIdx = -1;
};
