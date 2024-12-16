##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2024                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_library(gstreamer INTERFACE)

target_include_directories(
  gstreamer INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glib-2.0
  ${CMAKE_CURRENT_SOURCE_DIR}/include/gstreamer-1.0
)

if (WIN32)
  function (add_dependency name lib dll)
    add_library(gstreamer_dep_${name} SHARED IMPORTED)
    if ("${dll}" STREQUAL "")
    set_target_properties(gstreamer_dep_${name} PROPERTIES
      IMPORTED_IMPLIB
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/${lib}
    )
    else ()
    set_target_properties(gstreamer_dep_${name} PROPERTIES
      IMPORTED_IMPLIB
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/${lib}
      IMPORTED_LOCATION
        ${CMAKE_CURRENT_SOURCE_DIR}/dll/${dll}.dll
    )
    endif ()
    target_link_libraries(gstreamer INTERFACE gstreamer_dep_${name})
  endfunction()


  add_dependency(ass ass.lib "")
  add_dependency(avcodec avcodec.lib avcodec-60.dll)
  add_dependency(avfilter avfilter.lib avfilter-9.dll)
  add_dependency(avformat avformat.lib avformat-60.dll)
  add_dependency(avutil avutil.lib avutil-58.dll)
  add_dependency(bz2 bz2.lib bz2.dll)
  add_dependency(cairo cairo.lib cairo-2.dll)
  add_dependency(cairo-gobject cairo-gobject.lib cairo-gobject-2.dll)
  add_dependency(cairo-script-interpreter cairo-script-interpreter.lib cairo-script-interpreter-2.dll)
  add_dependency(charset charset.lib "")
  add_dependency(croco croco-0.6.lib "")
  add_dependency(crypto crypto.lib "")
  add_dependency(dav1d dav1d.lib dav1d.dll)
  add_dependency(dv dv.lib dv-4.dll)
  add_dependency(expat expat.lib "")
  add_dependency(ffi ffi.lib ffi-7.dll)
  add_dependency(FLAC FLAC.lib "")
  add_dependency(fontconfig fontconfig.lib fontconfig-1.dll)
  add_dependency(freetype freetype.lib freetype-6.dll)
  add_dependency(fribidi fribidi.lib fribidi-0.dll)
  add_dependency(gdk_pixbuf gdk_pixbuf-2.0.lib gdk_pixbuf-2.0-0.dll)
  add_dependency(ges ges-1.0.lib ges-1.0-0.dll)
  add_dependency(gio gio-2.0.lib gio-2.0-0.dll)
  add_dependency(glib glib-2.0.lib glib-2.0-0.dll)
  add_dependency(gmodule gmodule-2.0.lib gmodule-2.0-0.dll)
  add_dependency(gobject gobject-2.0.lib gobject-2.0-0.dll)
  add_dependency(graphene graphene-1.0.lib graphene-1.0-0.dll)
  add_dependency(gstadaptivedemux gstadaptivedemux-1.0.lib gstadaptivedemux-1.0-0.dll)
  add_dependency(gstadaptivedemux_2 gstadaptivedemux-1.0.lib gstadaptivedemux2.dll)
  add_dependency(gstallocators gstallocators-1.0.lib gstallocators-1.0-0.dll)
  add_dependency(gstapp gstapp-1.0.lib gstapp.dll)
  add_dependency(gstapp_2 gstapp-1.0.lib gstapp-1.0-0.dll)
  add_dependency(gstaudio gstaudio-1.0.lib gstaudio-1.0-0.dll)
  add_dependency(gstbadaudio gstbadaudio-1.0.lib gstbadaudio-1.0-0.dll)
  add_dependency(gstbase gstbase-1.0.lib gstbase-1.0-0.dll)
  add_dependency(gstbasecamerabinsrc gstbasecamerabinsrc-1.0.lib gstbasecamerabinsrc-1.0-0.dll)
  add_dependency(gstcheck gstcheck-1.0.lib gstcheck-1.0-0.dll)
  add_dependency(gstcodecparsers gstcodecparsers-1.0.lib gstcodecparsers-1.0-0.dll)
  add_dependency(gstcodecs gstcodecs-1.0.lib gstcodecs-1.0-0.dll)
  add_dependency(gstcontroller gstcontroller-1.0.lib gstcontroller-1.0-0.dll)
  add_dependency(gstcuda gstcuda-1.0.lib gstcuda-1.0-0.dll)
  add_dependency(gstd3d11 gstd3d11-1.0.lib gstd3d11.dll)
  add_dependency(gstd3d11_2 gstd3d11-1.0.lib gstd3d11-1.0-0.dll)
  add_dependency(gstdxva gstdxva-1.0.lib gstdxva-1.0-0.dll)
  add_dependency(gstfft gstfft-1.0.lib gstfft-1.0-0.dll)
  add_dependency(gstgl gstgl-1.0.lib gstgl-1.0-0.dll)
  add_dependency(gstinsertbin gstinsertbin-1.0.lib gstinsertbin.dll)
  add_dependency(gstinsertbin_2 gstinsertbin-1.0.lib gstinsertbin-1.0-0.dll)
  add_dependency(gstisoff gstisoff-1.0.lib gstisoff-1.0-0.dll)
  add_dependency(gstmpegts gstmpegts-1.0.lib gstmpegts-1.0-0.dll)
  add_dependency(gstmse gstmse-1.0.lib gstmse-1.0-0.dll)
  add_dependency(gstnet gstnet-1.0.lib gstnet-1.0-0.dll)
  add_dependency(gstpbutils gstpbutils-1.0.lib gstpbutils-1.0-0.dll)
  add_dependency(gstphotography gstphotography-1.0.lib gstphotography-1.0-0.dll)
  add_dependency(gstplay gstplay-1.0.lib gstplay-1.0-0.dll)
  add_dependency(gstplayer gstplayer-1.0.lib gstplayer-1.0-0.dll)
  add_dependency(gstreamer gstreamer-1.0.lib gstreamer-1.0-0.dll)
  add_dependency(gstriff gstriff-1.0.lib gstriff-1.0-0.dll)
  add_dependency(gstrtp gstrtp-1.0.lib gstrtp.dll)
  add_dependency(gstrtp_2 gstrtp-1.0.lib gstrtp-1.0-0.dll)
  add_dependency(gstrtsp gstrtsp-1.0.lib gstrtsp.dll)
  add_dependency(gstrtsp_2 gstrtsp-1.0.lib gstrtsp-1.0-0.dll)
  add_dependency(gstrtspserver gstrtspserver-1.0.lib gstrtspserver-1.0-0.dll)
  add_dependency(gstsctp gstsctp-1.0.lib gstsctp.dll)
  add_dependency(gstsctp_2 gstsctp-1.0.lib gstsctp-1.0-0.dll)
  add_dependency(gstsdp gstsdp-1.0.lib gstsdp-1.0-0.dll)
  add_dependency(gsttag gsttag-1.0.lib gsttaglib.dll)
  add_dependency(gsttag_2 gsttag-1.0.lib gsttag-1.0-0.dll)
  add_dependency(gsttranscoder gsttranscoder-1.0.lib gsttranscoder-1.0-0.dll)
  add_dependency(gsturidownloader gsturidownloader-1.0.lib gsturidownloader-1.0-0.dll)
  add_dependency(gstvalidate gstvalidate-1.0.lib gstvalidate-1.0-0.dll)
  add_dependency(gstvideo gstvideo-1.0.lib gstvideo-1.0-0.dll)
  add_dependency(gstwebrtc gstwebrtc-1.0.lib gstwebrtc.dll)
  add_dependency(gstwebrtc_2 gstwebrtc-1.0.lib gstwebrtc-1.0-0.dll)
  add_dependency(gstwebrtcnice gstwebrtcnice-1.0.lib gstwebrtcnice-1.0-0.dll)
  add_dependency(gstwinrt gstwinrt-1.0.lib gstwinrt-1.0-0.dll)
  add_dependency(gthread gthread-2.0.lib gthread-2.0-0.dll)
  add_dependency(harfbuzz harfbuzz.lib harfbuzz.dll)
  add_dependency(iconv iconv.lib libiconv-2.dll)
  add_dependency(intl intl.lib intl-8.dll)
  add_dependency(jpeg jpeg.lib "")
  add_dependency(json-glib json-glib-1.0.lib json-glib-1.0-0.dll)

  add_library(gstreamer_dep_thirdparty SHARED IMPORTED)
  set_target_properties(gstreamer_dep_thirdparty PROPERTIES
    IMPORTED_IMPLIB
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/gstreamer-1.0.lib
    IMPORTED_LOCATION
    "${CMAKE_CURRENT_SOURCE_DIR}/dll/libass-9.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libcharset-1.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libcroco-0.6-3.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libcrypto-1_1-x64.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libexpat-1.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libFLAC-8.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libgcc_s_seh-1.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libjpeg-8.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libmpg123-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libpng16-16.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/librsvg-2-2.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libspandsp-2.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libsrt.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libssl-1_1-x64.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libstdc++-6.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libturbojpeg-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libvorbis-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libvorbisenc-2.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libvorbisfile-3.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libwinpthread-1.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libxml2-2.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/libzbar-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/ltc-11.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/mp3lame-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/nice-10.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/ogg-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/openh264-6.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/openjp2.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/opus-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/orc-0.4-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/orc-test-0.4-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/pango-1.0-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/pangocairo-1.0-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/pangoft2-1.0-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/pangowin32-1.0-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/pcre2-8-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/pixman-1-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/postproc-57.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/psl-5.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/sbc-1.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/SoundTouch-1.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/soup-2.4-1.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/speex-1.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/sqlite3-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/srtp2-1.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/swresample-4.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/swscale-7.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/tag.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/theora-0.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/theoradec-1.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/theoraenc-1.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/tiff-6.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/wavpack.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/dll/z-1.dll"
  )
  target_link_libraries(gstreamer INTERFACE gstreamer_dep_thirdparty)

  add_library(gstreamer_dep_plugins SHARED IMPORTED)
  set_target_properties(gstreamer_dep_plugins PROPERTIES
    IMPORTED_IMPLIB
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/gstreamer-1.0.lib
    IMPORTED_LOCATION
      "${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaccurip.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstadder.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstadpcmdec.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstadpcmenc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaes.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaiff.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstalaw.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstalpha.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstalphacolor.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstamfcodec.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstamrnb.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstamrwbdec.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstanalytics-1.0-0.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstapetag.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstassrender.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudiobuffersplit.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudioconvert.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudiofx.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudiofxbad.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudiolatency.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudiomixer.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudiomixmatrix.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudioparsers.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudiorate.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudioresample.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudiotestsrc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaudiovisualizers.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstauparse.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstautoconvert.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstautodetect.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstavi.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstaws.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstbayer.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstbz2.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstcairo.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstcamerabin.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstcdg.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstclaxon.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstclosedcaption.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstcodecalpha.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstcodectimestamper.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstcoloreffects.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstcompositor.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstcoreelements.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstcoretracers.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstcutter.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstd3d.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstd3d12.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdash.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdav1d.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdebug.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdebugutilsbad.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdecklink.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdeinterlace.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdirectshow.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdirectsound.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdirectsoundsrc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdtls.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdtmf.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdv.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdvbsubenc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdvbsuboverlay.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdvdspu.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstdwrite.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsteffectv.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstencoding.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstequalizer.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstfallbackswitch.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstffv1.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstfieldanalysis.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstflac.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstflv.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstflxdec.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstfmp4.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstfreeverb.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstfrei0r.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstgaudieffects.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstgdkpixbuf.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstgdp.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstgeometrictransform.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstges.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstgif.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstgio.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstgoom.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstgoom2k1.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsthls.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsthlssink3.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsthsv.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsticydemux.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstid3demux.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstid3tag.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstimagefreeze.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstinter.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstinterlace.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstinterleave.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstipcpipeline.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstisac.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstisomp4.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstivfparse.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstivtc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstjack.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstjpeg.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstjpegformat.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstjson.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstladspa.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstlame.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstlegacyrawparse.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstlevel.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstlewton.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstlibav.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstlivesync.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstmatroska.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstmediafoundation.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstmidi.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstmp4.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstmpg123.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstmulaw.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstmultifile.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstmultipart.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstmxf.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstndi.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstnetsim.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstnice.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstnle.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstnvcodec.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstogg.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstopengl.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstopenh264.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstopenjpeg.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstopus.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstopusparse.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstoverlaycomposition.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstpango.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstpcapparse.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstplayback.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstpng.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstpnm.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstproxy.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstqroverlay.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstqsv.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstraptorq.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrav1e.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrawparse.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstregex.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstremovesilence.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstreplaygain.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstreqwest.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrfbsrc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrist.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrsaudiofx.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrsclosedcaption.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrsinter.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrsonvif.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrspng.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrsrtp.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrsrtsp.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrstracers.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrsvg.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrsvideofx.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrswebrtc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrtmp2.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrtpmanager.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrtpmanagerbad.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrtponvif.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstrtspclientsink.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsbc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsdpelem.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsegmentclip.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstshapewipe.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsiren.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsmooth.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsmoothstreaming.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsmpte.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsoundtouch.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsoup.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstspandsp.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstspectrum.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstspeed.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstspeex.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsrt.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsrtp.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsubenc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstsubparse.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstswitchbin.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsttcp.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsttextahead.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsttextwrap.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsttheora.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstthreadshare.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsttimecode.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsttogglerecord.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsttranscode.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsttypefindfunctions.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstudp.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsturiplaylistbin.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvideobox.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvideoconvertscale.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvideocrop.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvideofilter.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvideofiltersbad.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvideoframe_audiolevel.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvideomixer.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvideoparsersbad.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvideorate.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvideosignal.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvideotestsrc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvolume.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvorbis.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstvpx.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstwasapi.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstwasapi2.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstwavenc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstwavpack.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstwavparse.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstwebrtcdsp.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstwebrtchttp.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstwic.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstwin32ipc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstwinks.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstwinscreencap.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstxingmux.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsty4mdec.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gsty4menc.dll
      ${CMAKE_CURRENT_SOURCE_DIR}/dll/gstzbar.dll"
  )
  target_link_libraries(gstreamer INTERFACE gstreamer_dep_plugins)
endif ()
# else ()
#   # TODO: This should use find_package instead and be cleaned up
#   target_include_directories(
#     gstreamer SYSTEM INTERFACE
#     /usr/include
#     /usr/include/glib-2.0
#     /usr/lib/x86_64-linux-gnu/glib-2.0/include
#     /usr/include/gstreamer-1.0
#     /usr/lib/x86_64-linux-gnu/gstreamer-1.0/include
#     /usr/include/libsoup-2.4
#   )
# 
#   set(LIBS_SHARED
#     libglfw.so
#     libgstapp-1.0.so
#     libgstbase-1.0.so
#     libgstgl-1.0.so
#     libgstnvcodecc.so
#     libgstreamer-1.0.so
#     libgstsdp-1.0.so
#     libgstvideo-1.0.so
#     libgstwebrtc-1.0.so
#     libsoup-2.4.so.1
#     libbz2.so.1
#     libz.so
#     libharfbuzz.so
#     libglib-2.0.so
#     libgobject-2.0.so
#   )
#   foreach(LIB_NAME IN LISTS LIBS_SHARED)
#     cmake_print_variables(LIB_NAME)
#     find_library(LIB_FOUND ${LIB_NAME})
#     if(LIB_FOUND)
#       target_link_libraries(gstreamer PRIVATE ${LIB_FOUND})
#       message(STATUS "${LIB_NAME}_DIR: ${LIB_FOUND}")
#     else()
#       message(WARNING "Shared library ${LIB_NAME} not found")
#     endif()
#     unset(LIB_FOUND CACHE)
#   endforeach()
# endif ()
