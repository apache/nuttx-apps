include $(APPDIR)/Make.defs

# Program options

MODULE    = $(CONFIG_GAMES_NXDOOM)
PRIORITY  = $(CONFIG_GAMES_NXDOOM_PRIORITY)
STACKSIZE = $(CONFIG_GAMES_NXDOOM_STACKSIZE)
PROGNAME  = nxdoom

# Source files

COMMONSRCDIR = src
TEXTSCREENSRCDIR = textscreen
DOOMSRCDIR = $(COMMONSRCDIR)/doom
PCSOUNDSRCDIR = pcsound

# Includes

CFLAGS += -I$(COMMONSRCDIR) -I$(DOOMSRCDIR) -I$(TEXTSCREENSRCDIR)
CFLAGS += -I$(PCSOUNDSRCDIR)

# Source files

COMMONSRCS = i_system.c m_argv.c m_misc.c # Excluding i_main.c
COMMONSRCS += z_native.c # Native memory allocation

DEHACKEDSRCS = deh_io.c deh_main.c deh_mapping.c deh_text.c

BASESRCS =        \
  aes_prng.c      \
  d_event.c       \
  d_iwad.c        \
  d_loop.c        \
  d_mode.c        \
  deh_str.c       \
  gusconf.c       \
  i_glob.c        \
  i_input.c       \
  i_joystick.c    \
  i_timer.c       \
  i_video.c       \
  m_bbox.c        \
  m_cheat.c       \
  m_config.c      \
  m_controls.c    \
  m_fixed.c       \
  p_rejectpad.c   \
  memio.c         \
  tables.c        \
  v_diskicon.c    \
  v_video.c       \
  w_checksum.c    \
  w_main.c        \
  w_wad.c         \
  w_file.c        \
  w_file_stdc.c   \
  w_merge.c       \

# Only add ENDOOM stuff if the user wants it

ifeq ($(CONFIG_GAMES_NXDOOM_ENDOOM),y)
BASESRCS += i_endoom.c
endif

NETSRCS =         \
  net_client.c    \
  net_common.c    \
  net_dedicated.c \
  net_gui.c       \
  net_io.c        \
  net_loop.c      \
  net_packet.c    \
  net_query.c     \
  net_sdl.c       \
  net_server.c    \
  net_structrw.c  \

DOOMSRCS =     \
  am_map.c     \
  deh_ammo.c   \
  deh_bexstr.c \
  deh_cheat.c  \
  deh_doom.c   \
  deh_frame.c  \
  deh_misc.c   \
  deh_ptr.c    \
  deh_thing.c  \
  deh_weapon.c \
  d_items.c    \
  d_main.c     \
  d_net.c      \
  doomdef.c    \
  doomstat.c   \
  dstrings.c   \
  f_finale.c   \
  f_wipe.c     \
  g_game.c     \
  hu_lib.c     \
  hu_stuff.c   \
  info.c       \
  m_menu.c     \
  m_random.c   \
  p_ceilng.c   \
  p_doors.c    \
  p_enemy.c    \
  p_floor.c    \
  p_inter.c    \
  p_lights.c   \
  p_map.c      \
  p_maputl.c   \
  p_mobj.c     \
  p_plats.c    \
  p_pspr.c     \
  p_saveg.c    \
  p_setup.c    \
  p_sight.c    \
  p_spec.c     \
  p_switch.c   \
  p_telept.c   \
  p_tick.c     \
  p_user.c     \
  r_bsp.c      \
  r_data.c     \
  r_draw.c     \
  r_main.c     \
  r_plane.c    \
  r_segs.c     \
  r_sky.c      \
  r_things.c   \
  statdump.c   \
  st_lib.c     \
  st_stuff.c   \
  wi_stuff.c   

TXTSCREENSRCS =       \
  txt_conditional.c   \
  txt_checkbox.c      \
  txt_desktop.c       \
  txt_dropdown.c      \
  txt_fileselect.c    \
  txt_gui.c           \
  txt_inputbox.c      \
  txt_io.c            \
  txt_button.c        \
  txt_label.c         \
  txt_radiobutton.c   \
  txt_scrollpane.c    \
  txt_separator.c     \
  txt_spinctrl.c      \
  txt_sdl.c           \
  txt_strut.c         \
  txt_table.c         \
  txt_utf8.c          \
  txt_widget.c        \
  txt_window.c        \
  txt_window_action.c 

CSRCS += $(patsubst %,$(COMMONSRCDIR)/%,$(COMMONSRCS))
CSRCS += $(patsubst %,$(COMMONSRCDIR)/%,$(DEHACKEDSRCS))
CSRCS += $(patsubst %,$(COMMONSRCDIR)/%,$(BASESRCS))
CSRCS += $(patsubst %,$(DOOMSRCDIR)/%,$(DOOMSRCS))

# Textscreen stuff only needs to be included for networking stuff
# or if the user wants to see ENDOOM

ifeq ($(CONFIG_GAMES_NXDOOM_ENDOOM),y)
ADDTXTSCREEN = y
endif

ifeq ($(CONFIG_GAMES_NXDOOM_NET),y)
ADDTXTSCREEN = y
endif

# Only include textscreen code if needed

ifeq ($(ADDTXTSCREEN),y)
CSRCS += $(patsubst %,$(TEXTSCREENSRCDIR)/%,$(TXTSCREENSRCS))
endif

# Only include networking logic if enabled

ifeq ($(CONFIG_GAMES_NXDOOM_NET),y)
CSRCS += $(patsubst %,$(COMMONSRCDIR)/%,$(NETSRCS))
endif

# Only include sound logic if enabled

ifeq ($(CONFIG_GAMES_NXDOOM_SOUND),y)
CSRCS += $(COMMONSRCDIR)/i_musicpack.c
CSRCS += $(COMMONSRCDIR)/i_pcsound.c
CSRCS += $(COMMONSRCDIR)/i_sdlmusic.c
CSRCS += $(COMMONSRCDIR)/i_sound.c
CSRCS += $(COMMONSRCDIR)/midifile.c
CSRCS += $(COMMONSRCDIR)/mus2mid.c

CSRCS += $(DOOMSRCDIR)/sounds.c
CSRCS += $(DOOMSRCDIR)/s_sound.c
CSRCS += $(DOOMSRCDIR)/deh_sound.c
endif

MAINSRC  = $(COMMONSRCDIR)/i_main.c

include $(APPDIR)/Application.mk
