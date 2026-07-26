#pragma once

#include "layer.hpp"

#include <vector>

namespace paint
{
    // Undo/redo as full layer-stack pixel snapshots. Simple and correct, at the
    // cost of O(depth * layers * width * height * 4 bytes) memory — depth is
    // capped (default 20) to bound that instead of trying to diff/compress.
    class HistoryStack
    {
    public:
        explicit HistoryStack(size_t maxDepth = 20);

        // Callers capture the state *before* mutating the canvas and push it
        // here, e.g.: `history.push(canvas.captureState()); /* then mutate */`.
        // Starting a new action always clears the redo stack.
        void push(HistoryState stateBeforeAction);

        bool canUndo() const;
        bool canRedo() const;

        void undo(Canvas& canvas);
        void redo(Canvas& canvas);

    private:
        size_t m_maxDepth;
        std::vector<HistoryState> m_undoStack;
        std::vector<HistoryState> m_redoStack;
    };
} // namespace paint
