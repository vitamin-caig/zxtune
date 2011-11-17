#!/usr/bin/ruby

require 'RMagick'

def save_icon(img, offset_y, size, position, name)
	img.crop((size + 1) * position + 1, offset_y, size, size).write("icons/#{name}")
	puts "#{name} saved"
end

img = Magick::ImageList.new(ARGV[0])

save_icon(img, 1, 16, 0, 'app_about.png')
save_icon(img, 1, 16, 1, 'app_aboutqt.png')
save_icon(img, 1, 16, 2, 'app_components.png')
save_icon(img, 1, 16, 3, 'app_exit.png')

save_icon(img, 18, 24, 0, 'ctrl_next.png')
save_icon(img, 18, 24, 1, 'ctrl_pause.png')
save_icon(img, 18, 24, 2, 'ctrl_play.png')
save_icon(img, 18, 24, 3, 'ctrl_prev.png')
save_icon(img, 18, 24, 4, 'ctrl_stop.png')

save_icon(img, 43, 16, 0, 'list_addfiles.png')
save_icon(img, 43, 16, 1, 'list_addfolder.png')
save_icon(img, 43, 16, 2, 'list_clear.png')
save_icon(img, 43, 16, 3, 'list_close.png')
save_icon(img, 43, 16, 4, 'list_create.png')
save_icon(img, 43, 16, 5, 'list_looped.png')
save_icon(img, 43, 16, 6, 'list_open.png')
save_icon(img, 43, 16, 7, 'list_random.png')
save_icon(img, 43, 16, 8, 'list_save.png')
save_icon(img, 43, 16, 9, 'list_crop.png')

save_icon(img, 43, 16, 10, 'setup_interpolation_off.png')
save_icon(img, 43, 16, 11, 'setup_interpolation_on.png')
save_icon(img, 43, 16, 12, 'setup_type_ay.png')
save_icon(img, 43, 16, 13, 'setup_type_ym.png')

save_icon(img, 60, 16, 0, 'pb_next.png')
save_icon(img, 60, 16, 1, 'pb_pause.png')
save_icon(img, 60, 16, 2, 'pb_play.png')
save_icon(img, 60, 16, 3, 'pb_prev.png')
save_icon(img, 60, 16, 4, 'pb_stop.png')

save_icon(img, 60, 16, 5, 'list_delduplicates.png')
save_icon(img, 60, 16, 6, 'list_rename.png')
save_icon(img, 60, 16, 7, 'list_export.png')
save_icon(img, 60, 16, 8, 'list_statistic.png')
save_icon(img, 60, 16, 9, 'list_group.png')
save_icon(img, 60, 16, 10, 'list_selripoffs.png')
