<?php
// $Id$

function os_publications_overview() {
  $args = func_get_args();

  if (count($args) == 0) {
    // get default types the user has selected
    $defaults = variable_get('os_publications_filter_publication_types', array());
    if (count(array_filter($defaults))) {
      $args = array('type', array_filter($defaults));
    }
  }

  $ckey = _os_publications_cache_key($args);
  if ($cache = cache_get($ckey, 'cache_biblio_lists')) {
    return $cache->data;
  }

  module_load_include('inc', 'biblio', 'includes/biblio.pages');
  $output = call_user_func_array('biblio_page', $args);

  // remove the filter status message
  unset($output['biblio_page']['header']['filter_status']);

  // remove the sorting option
  unset($output['biblio_page']['header']['sort_links']);

  // remove the alphabetic filtering
  unset($output['biblio_page']['header']['alpha_line']);

  // handle caching
  cache_set($ckey, $output, 'cache_biblio_lists');

  return $output;
}

function _os_publications_cache_key($args) {
  $info = array(
    'func_args' => $args,
    'uri' => drupal_parse_url(request_uri()),
  );
  module_load_include('inc', 'biblio', 'includes/biblio.pages');
  $arg_list = biblio_arg_handler($info);

  $key = array();
  $key[] = 'biblio';

  if (module_exists('spaces') && $space = spaces_get_space()) {
    $key[] = 'space';
    $key[] = $space->id;
  }

  $has_arg = false;
  if (isset($arg_list['f'])) {
    $has_arg = true;
    foreach ($arg_list['f'] as $l => $d) {
      if (is_array($d)) {
        foreach ($d as $v) {
          $key[] = $l;
          $key[] = $v;
        }
      }
      else {
        $key[] = $l;
        $key[] = $d;
      }
    }
  }

  if (isset($arg_list['s'])) {
    $has_arg = true;
    $key[] = 'sort';
    $key[] = $arg_list['s'];
  }

  if (isset($arg_list['o'])) {
    $has_arg = true;
    $key[] = 'order';
    $key[] = $arg_list['o'];
  }

  if (!$has_arg) {
    $key[] = 'args';
    $key[] = 'no';
  }

  if (isset($_GET['page'])) {
    $key[] = 'page';
    $key[] = $_GET['page'];
  }

  return implode(':', $key);
}

function os_publications_export($format = "tagged", $nid = NULL, $popup = FALSE) {
  module_load_include('inc', 'biblio', 'includes/biblio.import.export');

  $title = 'publications';
  if ($nid !== NULL) {
    $node  = node_load($nid);
    $title = str_replace(' ', '_', $node->title);
    $title = substr($title, 0, 8);
  }
  elseif (empty($_SESSION['last_biblio_query'])) {
    // Biblio uses this session value, if this is an anon user that has no session (or a cli user) lets set this up for them
    $args = array();
    // get default types the user has selected
    $defaults = variable_get('os_publications_default_types', array());
    if (count($defaults)) {
      $args = array('type', $defaults);
    }
    // Setup the default query
    module_load_include('inc', 'biblio', 'includes/biblio.pages');
    biblio_build_query($args);
  }

  ob_start();
  $output = biblio_export($format, $nid, $popup);
  if (empty($output)) {
    // the export didn't return anything, which means it output directly to the browser
    $output = ob_get_clean();

    // handle for spaces
    if (module_exists('spaces') && $space = spaces_get_space()) {
      $title = $space->group->purl . '-' . $title;
    }

    $header = drupal_get_http_header('Content-Disposition');

    $match = array();
    preg_match('!filename="([^"]+)"!', $header, $match);
    $filename = str_replace(array('Drupal-Biblio', 'Biblio'), $title, $match[1]);
    $header_new = preg_replace('!filename="([^"]+)"!', 'filename="' . $filename . '"', $header);

    drupal_add_http_header('Content-Disposition', $header_new);
    echo $output;
  }
  else {
    ob_end_clean();
    return $output;
  }
}

function os_publications_settings() {
  $style_name = biblio_get_style();

  $form['left'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('column', 'column-left'),
    ),
  );
  $form['right'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('column', 'column-right'),
    ),
  );
  $style = module_exists('biblio_citeproc') ? 'biblio_citeproc_style' : 'biblio_style';
  $form['left'][$style] = array(
    '#type' => 'radios',
    '#title' => t('Select your preferred bibliographic format'),
    //'#description' => t('This alters the bibliographic format of your publications.'),
    '#default_value' => biblio_get_style(),
    '#options' => biblio_get_styles(),
  );

  $form['right']['biblio_export_links'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Select which export formats should accompany each bibliographic record'),
    '#default_value' => variable_get('biblio_export_links', array('tagged', 'xml', 'bibtex')),
    '#options' => array(
      'tagged' => t('EndNote Tagged'),
      'xml' => t('EndNote XML'),
      'bibtex' => t('BibTex'),
    ),
    '#description' => t('Each selected format will appear in its own export link.'),
  );

  $form['right']['biblio_google_link'] = array(
    '#type' => 'checkbox',
    '#title' => t('Link to Google Scholar'),
    '#return_value' => 1,
    '#default_value' => variable_get('biblio_google_link', 1),
    '#description' => t('This adds the necessary metadata to your web site to allow Google Scholar to properly index your publication.'),
  );

  //citation distribute plugins
  /*
   $form['right']['citation_distribute_autoflags'] = array(
     '#title' => t('Automatic Node Flagging'),
     '#type' => 'checkboxes',
     '#description' => t('New publications will automatically be flagged for distribution to the selected services.'),
     '#default_value' => variable_get('citation_distribute_autoflags', array()),
     '#options' => array(),
   );
   foreach (_citation_distribute_plugins() as $plugin) {
     if (isset($plugin['title'])) {
       $form['citation_distribute_autoflags']['#options'] += array($plugin['handler']['class'] => $plugin['title']);
     }
   }*/

  $form['right']['os_publications_note_in_teaser'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show the "note" content in teaser?'),
    //'#description' => t('This alters the bibliographic format of your publications.'),
    '#default_value' => variable_get('os_publications_note_in_teaser', FALSE),
    '#description' => t('If this is checked and you add a note to a publication then it will show as part of a publications teaser.'),
  );

  $form['left']['biblio_rowsperpage'] = array(
    '#type' => 'textfield',
    '#title' => t('Publications per page'),
    '#size' => 5,
    '#default_value' => variable_get('biblio_rowsperpage', 25),
    '#description' => t('Select the number of publication to be displayed on each page.'),
  );

  $form['left']['biblio_sort'] = array(
    '#type' => 'radios',
    '#title' => t('By default, sort publications by which category'),
    '#default_value' => variable_get('biblio_sort', 'year'),
    '#options' => array('author' => t('Author'), 'title' => t('Title'), 'type' => t('Type'), 'year' => t('Year')),
  );

  $form['left']['biblio_order'] = array(
    '#type' => 'radios',
    '#title' => t('Select default order of the sort category'),
    '#default_value' => variable_get('biblio_order', 'DESC'),
    '#options' => array('DESC' => t('Descending'), 'ASC' => t('Ascending')),
  );

  $form['left']['biblio_sort_tabs'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Allow additional sorting of your publications by'),
    '#default_value' => variable_get('biblio_sort_tabs', array('author' => 'author', 'title' => 'title', 'type' => 'type', 'year' => 'year')),
    '#options' => array('author' => t('Author'), 'title' => t('Title'), 'type' => t('Type'), 'year' => t('Year')),
    //'#description' => t('You turn the sorting links at the top of the /biblio page here.'),
  );

  $result = db_query('SELECT t.* FROM {biblio_types} as t WHERE tid > -1 AND visible = 1');
  foreach ($result as $option) {
    $results[$option->tid] = $option->name;
  }
  asort($results);

  $form['right']['os_publications_filter_publication_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Show only publications of type'),
    '#default_value' => variable_get('os_publications_filter_publication_types', array_keys($results)),
    '#options' => $results,
    '#description' => t('The main list of publications on your publications page will list the types slected here.'),
    '#attributes' => array('class' => array('form-checkboxes-scrolling')),
  );

  //Should urls in citations be shortened?
/*  $form['scholar_publications'] = array(
  '#type' => 'fieldset',
  );
*/
  $form['right']['os_publications_shorten_citations'] = array(
    '#type' => 'checkbox',
    '#title' => t('Short URLs in citations'),
    '#description' => t('When this is checked, a shortened URL will be included in the citation.'),
    '#default_value' => variable_get('os_publications_shorten_citations'),
  );

  return system_settings_form($form);
}

function os_publications_reset(&$form, &$form_state) {
  $form['help'] = array(
    '#markup' => t('Select tables to reset:'),
  );

  $form['biblio_field_type'] = array(
    '#type' => 'checkbox',
    '#title' => t('Field Settings'),
    '#description' => t('Will update standard publication types to our defaults. Will not affect publication types you have added.'),
  );

  $form['biblio_contributor_type'] = array(
    '#type' => 'checkbox',
    '#title' => t('Contributor Settings'),
    '#description' => t('Will completely replace all Contributor settings.'),
  );

  $form['actions'] = array(
    '#type' => 'actions',
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Go'),
    )
  );

  $form['#submit'][] = 'os_publications_reset_submit';
}

function os_publications_reset_submit($form, $form_state) {
  dpm($form_state['values']);
}
