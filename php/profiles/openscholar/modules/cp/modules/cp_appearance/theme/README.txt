cp_appearance
==================================

cp_appearance provides an interface to the OpenScholar control panel for
selecting themes and theme flavors.

Using themes in cp_appearance
----------------------------------

To tell cp_appearance that your theme is usable as an OS site, its info file should include the following line:
  os[theme_type] = 'vsite'

### Privacy

@todo privacy/access

os[theme_access]

Flavors
-----------------------------------
flavors
plugins[os][flavor] = flavors //directory to flavors