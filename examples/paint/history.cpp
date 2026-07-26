#include "history.hpp"

#include <utility>

namespace paint
{
    HistoryStack::HistoryStack(size_t maxDepth)
        : m_maxDepth(maxDepth)
    {
    }

    void HistoryStack::push(HistoryState stateBeforeAction)
    {
        m_undoStack.push_back(std::move(stateBeforeAction));
        if (m_undoStack.size() > m_maxDepth) {
            m_undoStack.erase(m_undoStack.begin());
        }
        m_redoStack.clear();
    }

    bool HistoryStack::canUndo() const { return not m_undoStack.empty(); }
    bool HistoryStack::canRedo() const { return not m_redoStack.empty(); }

    void HistoryStack::undo(Canvas& canvas)
    {
        if (m_undoStack.empty()) {
            return;
        }

        m_redoStack.push_back(canvas.captureState());
        HistoryState previous = std::move(m_undoStack.back());
        m_undoStack.pop_back();
        canvas.restoreState(previous);
    }

    void HistoryStack::redo(Canvas& canvas)
    {
        if (m_redoStack.empty()) {
            return;
        }

        m_undoStack.push_back(canvas.captureState());
        HistoryState next = std::move(m_redoStack.back());
        m_redoStack.pop_back();
        canvas.restoreState(next);
    }
} // namespace paint
