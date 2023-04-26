package app.zxtune.ui.utils

import android.view.*
import android.widget.FrameLayout
import androidx.appcompat.widget.Toolbar
import androidx.core.animation.Animator
import androidx.core.animation.AnimatorListenerAdapter
import androidx.core.animation.AnimatorSet
import androidx.core.animation.ObjectAnimator
import androidx.core.view.MenuProvider
import androidx.core.view.get
import androidx.recyclerview.selection.Selection
import androidx.recyclerview.selection.SelectionTracker
import app.zxtune.R

class SelectionUtils<T : Any>(
    parent: ViewGroup, private val tracker: SelectionTracker<T>, private val client: Client<T>
) : SelectionTracker.SelectionObserver<T>() {
    interface Client<T> {
        fun getTitle(count: Int): String
        val allItems: List<T>

        fun fillMenu(inflater: MenuInflater, menu: Menu)
        fun processMenu(itemId: Int, selection: Selection<T>): Boolean
    }

    private val selection
        get() = tracker.selection

    private val action: ActionLayout by lazy {
        ActionLayout(parent) {
            setNavigationOnClickListener {
                tracker.clearSelection()
            }
            addMenuProvider(ClientMenuProvider())
            addMenuProvider(SelectionMenuProvider())
        }
    }

    override fun onSelectionChanged() {
        val count = selection.size()
        if (count != 0) {
            action.isVisible = true
            action.title = client.getTitle(count)
        } else {
            action.isVisible = false
        }
    }

    override fun onSelectionRestored() = onSelectionChanged()

    private class ActionLayout(
        parent: ViewGroup,
        setupToolbar: Toolbar.() -> Unit,
    ) {
        init {
            require(parent.childCount == 1)
        }

        private val userContent = parent[0]
        private val panel = LayoutInflater.from(parent.context).inflate(
            R.layout.selection, parent, false
        ) as Toolbar

        private val animation = AnimatorSet().apply {
            playSequentially(
                ObjectAnimator.ofFloat(userContent, View.ALPHA, 1f, 0f),
                ObjectAnimator.ofFloat(panel, View.ALPHA, 0f, 1f)
            )
            addListener(object : AnimatorListenerAdapter() {
                override fun onAnimationStart(animation: Animator, isReverse: Boolean) {
                    userContent.show()
                    panel.show()
                }

                override fun onAnimationEnd(animation: Animator, isReverse: Boolean) {
                    if (isReverse) {
                        userContent.show()
                        panel.hide()
                    } else {
                        panel.show()
                        userContent.hide()
                    }
                }
            })
        }

        var isVisible: Boolean = false
            set(visible) {
                if (field != visible) {
                    field = visible
                    if (visible) {
                        animation.start()
                    } else {
                        animation.reverse()
                    }
                }
            }

        var title: CharSequence by panel::title

        init {
            setupToolbar(panel)
            panel.run {
                alpha = 0f
                visibility = View.GONE
            }
            parent.addView(panel)
        }
    }

    private inner class ClientMenuProvider : MenuProvider {
        override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) =
            client.fillMenu(menuInflater, menu)

        override fun onMenuItemSelected(menuItem: MenuItem) =
            if (client.processMenu(menuItem.itemId, selection)) {
                tracker.clearSelection()
                true
            } else {
                false
            }
    }

    private inner class SelectionMenuProvider : MenuProvider {
        override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) =
            menuInflater.inflate(R.menu.selection, menu)

        override fun onMenuItemSelected(menuItem: MenuItem) = when (menuItem.itemId) {
            R.id.action_select_all -> {
                tracker.setItemsSelected(client.allItems, true)
                true
            }

            R.id.action_select_none -> {
                tracker.clearSelection()
                true
            }

            R.id.action_select_invert -> {
                invertSelection(client.allItems, tracker)
                true
            }

            else -> false
        }
    }

    companion object {
        @JvmStatic
        fun <T : Any> install(
            parent: FrameLayout, tracker: SelectionTracker<T>, client: Client<T>
        ) {
            tracker.addObserver(SelectionUtils(parent, tracker, client))
        }

        private fun <T : Any> invertSelection(items: List<T>, tracker: SelectionTracker<T>) {
            val (selected, unselected) = items.partition(tracker::isSelected)
            tracker.setItemsSelected(unselected, true)
            tracker.setItemsSelected(selected, false)
        }
    }
}

private fun View.show() {
    visibility = View.VISIBLE
}

private fun View.hide() {
    visibility = View.GONE
}
