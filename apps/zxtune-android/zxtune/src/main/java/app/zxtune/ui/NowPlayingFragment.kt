/**
 * @file
 * @brief Now playing fragment component
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.widget.Toolbar
import androidx.fragment.app.Fragment
import app.zxtune.MainActivity.PagerTabListener
import app.zxtune.R

class NowPlayingFragment : Fragment(), PagerTabListener {

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let {
        inflater.inflate(R.layout.now_playing, it, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        view.findViewById<Toolbar>(R.id.main_top_panel)
            .addMenuProvider(ApplicationMenu(requireActivity()))
    }

    override fun onTabVisibilityChanged(isVisible: Boolean) {
        requireView().findViewById<View>(R.id.spectrum).visibility =
            if (isVisible) View.VISIBLE else View.INVISIBLE
    }
}
