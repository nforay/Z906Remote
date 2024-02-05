/**
 * plugins/vuetify.ts
 *
 * Framework documentation: https://vuetifyjs.com`
 */

// Styles
//import "@mdi/font/css/materialdesignicons.css";
import 'vuetify/styles'

// Composables
import { createVuetify } from 'vuetify'
import { mdi } from 'vuetify/iconsets/mdi-svg'
import {
  mdiSpeaker,
  mdiTune,
  mdiAudioInputRca,
  mdiSurroundSound,
  mdiDolby,
  mdiPower,
  mdiVolumeVariantOff,
  mdiVolumeOff,
  mdiVolumeMinus,
  mdiVolumePlus,
  mdiRadioboxBlank,
  mdiRadioboxMarked,
} from '@mdi/js'

// https://vuetifyjs.com/en/introduction/why-vuetify/#feature-guides
export default createVuetify({
  theme: {
    defaultTheme: 'dark',
    themes: {
      light: {
        colors: {
          primary: '#1867C0',
          secondary: '#5CBBF6',
        },
      },
    },
  },
  icons: {
    defaultSet: 'mdi',
    aliases: {
      speaker: mdiSpeaker,
      tune: mdiTune,
      rca: mdiAudioInputRca,
      surround: mdiSurroundSound,
      dolby: mdiDolby,
      power: mdiPower,
      muteOn: mdiVolumeVariantOff,
      muteOff: mdiVolumeOff,
      volMinus: mdiVolumeMinus,
      volPlus: mdiVolumePlus,
      radioFalse: mdiRadioboxBlank,
      radioTrue: mdiRadioboxMarked,
    },
    sets: {
      mdi,
    },
  },
})
