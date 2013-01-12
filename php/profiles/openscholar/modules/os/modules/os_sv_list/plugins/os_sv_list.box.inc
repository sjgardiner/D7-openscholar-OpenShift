<?php
class os_sv_list_box extends os_boxes_default {

  /**
   * Implementation of boxes_content::options_defaults().
   */
  public function options_defaults() {
    $options = parent::options_defaults();
    $biblio_types = $this->_biblio_types();
    $biblio_whitelist = array_combine(array_keys($biblio_types), array_keys($biblio_types));

    $options += array(
      'content_type' => 'all',
      'display' => 'display_title',
      'more' => FALSE,
      'more_text' => t('More'),
      'more_link' => '', // what's a good default?

      'pager' => FALSE,
      'number_of_items' => 6,
      'sort_by' => 'sort_newest',
      'vocabs' => array(),
      'terms' => array(),

      'biblio_whitelist' => $biblio_whitelist,
    );

    return $options;
  }

  /**
   * Implementation of boxes_content::options_form().
   */
  public function options_form(&$form_state) {
    $form = parent::options_form($form_state);
    $defaults = $this->options_defaults();
    drupal_add_js(array('os_sv_list_box' => array('new_box' => $this->new)), 'setting');

    $content_types = array(
      'all' => t('All')
    ) + os_get_bundles(array(1));

    if (isset($content_types['media_gallery'])) {
      $content_types['image'] = t('Images');
    }

    $form['#submit'] = array('os_sv_list_form_submit');

    $content_types_value = !empty($form_state['values']['content_type']) ? $form_state['values']['content_type'] : $this->options['content_type'];

    $form['content_type'] = array(
      '#type' => 'select',
      '#title' => t('Content Type'),
      '#default_value' => $content_types_value,
      '#options' => $content_types,
      '#id' => 'os_sv_list_content_type',
    /*
      '#ajax' => array(
        'callback' => 'os_sv_list_more_link_ajax',
        //'wrapper' => 'more_link_div',
        'wrapper' => 'edit-more-link',
        'method' => 'replace',
        'effect' => 'none',
      ),*/
      '#attached' => array(
        'js' => array(
          drupal_get_path('module', 'os_sv_list').'/os_sv_list.js',
        ),
      ),
    );

    /*$form['biblio_show_all'] = array(
      '#type' => 'checkbox',
      '#title' => t('Show all publication types'),
      '#default_value' => isset($this->options['biblio_show_all']) ? $this->options['biblio_show_all'] : $defaults['biblio_show_all'],
      '#id' => 'biblio_show_all_check',
      '#prefix' => '<div id="os_sv_list_biblio_show_all">',
      '#suffix' => '</div>',
    );*/

    $form['biblio_whitelist'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Select publication types to display'),
      '#default_value' => isset($this->options['biblio_whitelist']) ? $this->options['biblio_whitelist'] : $defaults['biblio_whitelist'],
      '#options' => $this->_biblio_types(),
      '#prefix' => '<div id="os_sv_list_biblio_whitelist">',
      '#suffix' => '</div>',
      '#attached' => array(
        'js' => array(
          drupal_get_path('module', 'os_boxes').'/misc/selectAll.js',
        ),
      ),
      '#states' => array(
        'visible' => array(
          '#os_sv_list_content_type' => array('value' => 'biblio'),
        ),
        'invisible' => array(
          '#os_sv_list_content_type' => array('!value' => 'biblio')
        ,)
      )
    );

    $form['display'] = array(
      '#type' => 'select',
      '#title' => 'Display style',
      '#default_value' => $this->options['display'],
      '#options' => array(
        'display_title' => t('Title'),
        'display_teaser' => t('Teaser'),
        'display_body' => t('Entire Post')
      )
    );

    $form['sort_by'] = array(
      '#type' => 'select',
      '#title' => 'Sorting',
      '#default_value' => $this->options['sort_by'],
      '#options' => array(
        //content type sensitive sorts
        'sort_event' => t('Event Date'),
        'sort_presentation' => t('Recently Presented'),
        'sort_biblio' => t('Year of Publication'),

        //standard sorts
        'sort_newest' => t('Newest Post'),
        'sort_oldest' => t('Oldest Post'),
        'sort_alpha' => t('Alphabetical'),
      )
    );

    $form['query_details'] = array(
      '#prefix' => '<div id="query-details">',
      '#suffix' => '</div>',
      '#parents' => array('query_details'),
    );

    if ($vocabs = os_sv_list_get_content_type_vocabs($content_types_value)) {
      // Make the vocabulary selection optional.
      $vocabs = array(-1 => t('- Select -')) + $vocabs;
    }
    $vocabs_default_value = !empty($form_state['values']['vocabs']) ? $form_state['values']['vocabs'] : $this->options['vocabs'];
    $terms_default_value = !empty($form_state['values']['terms']) ? $form_state['values']['terms'] : $this->options['terms'];

    $form['query_details']['vocabs'] = array(
      '#type' => 'select',
      '#title' => t('Vocabulary'),
      '#description' => t('Select a vocabulary to filter content by tagged items.'),
      '#options' => $vocabs,
      '#default_value' => $vocabs_default_value,
      '#access' => $vocabs,
      '#ajax' => array(
        'callback' => 'os_sv_list_terms_details_ajax',
        'wrapper' => 'terms-details',
      ),
    );

    $terms = array();
    if ($vocabs_default_value) {
      $tree = taxonomy_get_tree($vocabs_default_value);
      foreach ($tree as $item) {
        $term = taxonomy_term_load($item->tid);
        $terms[$term->tid] = $term->name;
      }
    }

    $form['query_details']['terms'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Vocabulary terms'),
      '#description' => empty($terms) ? t('No terms in the vocabulary') : '',
      '#options' => $terms,
      '#access' => $vocabs,
      '#states' => array(
        'invisible' => array(
          ':input[name="vocabs"]' => array('value' => -1),
        ),
      ),
      '#default_value' => $terms_default_value,
      '#prefix' => '<div id="terms-details">',
      '#suffix' => '</div>',
    );

    $terms_statement = isset($form_state['values']['content_type']) && $form_state['values']['content_type'] == -1;
    $vocabs_statement = isset($form_state['values']['vocabs']) && $form_state['values']['vocabs'] == -1;

    // Hide the form but keep the prefix and suffix for the AJAX callback.
    if ($terms_statement || $vocabs_statement) {
      $form['query_details']['terms'] = array(
        '#prefix' => '<div id="terms-details">',
        '#suffix' => '</div>',
      );
    }

    $form['query_details']['more'] = array(
      '#title' => 'Show "more" link',
      '#type' => 'checkbox',
      '#default_value' => $this->options['more'],
    );

    $content_map = array(
      'all' => '',
      'news' => 'news',
      'biblio' => 'publications',
      'event' => 'calendar',
      'page' => 'pages',
      'data' => 'data',
      'blog' => 'blog',
      'book' => 'documents',
      'feed' => 'reader',
      'media_gallery' => 'galleries',
      'link' => 'links',
      'presentation' => 'presentations',
      'software_project' => 'software',
      'software_release' => 'software',
      'person' => 'people',
    );

    if (!empty($content_map[$content_types_value]) && (empty($this->options['more_link']) || in_array($this->options['more_link'], $content_map))) {
      $default_value = $content_map[$content_types_value];
    }
    else {
      $default_value = $this->options['more_link'];
    }

    if (!empty($form_state['values']['more_link'])) {
      // Unset the values from the text field, as otherwise FAPI will add
      // it to the #value of the form-element.
      // @see http://drupal.org/node/1024962#comment-4021826
      unset($form_state['input']['more_link']);
    }

    $form['query_details']['more_link'] = array(
      '#title' => t('URL for the more link'),
      '#type' => 'textfield',
      '#default_value' => $default_value,
      '#states' => array(
        'visible' => array('input[name=more]' => array('checked' => TRUE)),
        'invisible'  => array('input[name=more]' => array('checked' => FALSE)),
      ),
      '#prefix' => '<div id="more_link_div">',
      '#suffix' => '</div>',
      '#attached' => array(
        'js' => array(
          array(
            'data' => array(
              'more_link_defaults' => array(
                'all' => '',
                'news' => 'news',
                'biblio' => 'publications',
                'event' => 'calendar',
                'page' => 'pages',
                'data' => 'data',
                'blog' => 'blog',
                'book' => 'documents',
                'feed' => 'reader',
                'media_gallery' => 'galleries',
                'link' => 'links',
                'presentation' => 'presentations',
                'software_project' => 'software',
                'software_release' => 'software',
                'person' => 'people',
              ),
            ),
            'type' => 'setting',
          )
        )
      )
    );

    $form['query_details']['more_text'] = array(
      '#title' => 'Label of the more link (default: "more")',
      '#type' => 'textfield',
      '#default_value' => $this->options['more_text'],
      '#states' => array(
        'visible' => array('input[name=more]' => array('checked' => TRUE))
      )
    );

    $form['pager'] = array(
      '#title' => 'Show pager',
      '#type' => 'checkbox',
      '#default_value' => $this->options['pager']
    );

    $form['number_of_items'] = array(
      '#title' => 'Number of items to display',
      '#type' => 'select',
      '#options' => array_slice(range(0, 20), 1, NULL, TRUE),
      '#default_value' => $this->options['number_of_items']
    );

    return $form;
  }
  /**
   * // * Implementation of boxes_content::render().
   */
  public function render() {
    $block = parent::render();
    $options = $this->options;
    $page_num = isset($_GET['page']) ? $_GET['page'] : 0;
    $page = 0;


    $nids = $this->get_nids($page_num, $page);
    $more_link = FALSE;

    if (count($nids)) {
      if (count($nids) >= $options['number_of_items']) {
        $nids = array_slice($nids, 0, $options['number_of_items']);
        $more_link = TRUE;
      }

      $display_func = (method_exists($this, $options['display'])) ? $options['display'] : 'display_title';
      $nodes = node_load_multiple($nids);

      if ($options['content_type'] == 'image') {
        $display_func = 'display_image';
        $nodes = file_load_multiple($nids);
      }

      $block['content'] = $this->$display_func($nodes);
    }

    // pager widget
    $pager = '';
    if ($options['pager'] && $block['content']) {
      // theme_pager();
      $parameters = array(
        'sv_list_box_delta' => $this->delta,
        'pager_id' => $page,
      );
      $pager = theme('views_mini_pager', array(
        'parameters' => $parameters,
        'element' => $page,
        'quantity' => $options['number_of_items'],
      ));
    }

    // more link
    if ($options['more'] && $more_link && $block['content']) {
      $block['content'] .= l($options['more_text'], $options['more_link'], array('attributes' => array('class' => array('more-link'))));
    }

    drupal_add_library('system', 'drupal.ajax');
    drupal_add_js(drupal_get_path('module', 'os_sv_list'). '/os_sv_list_display.js');

    if ($block['content']) {
      $block['content'] = '<div id="box-' . $this->delta . '-page" data-page="'.$page_num.'" data-delta="'.$this->delta.'">' . $block['content'] . '</div>' . $pager;
      return $block;
    } else {
      $block['title'] = $block['subject'] = '';
      return $block;
    }

  }

  // get_default_query_query - humble beginnings of the EFQ
  function get_default_query() {
    $bundles = os_get_bundles(array(1));
    if (empty($bundles)) {
      return FALSE;
    }

    // defaults
    $efq = new EntityFieldQuery();
    $efq->entityCondition('entity_type', 'node');
    $efq->propertyCondition('status', 1);
    $efq->propertyCondition('type', array_keys($bundles), 'IN');

    // vsite
    if (module_exists('vsite') && $space = spaces_get_space()) {
      $efq->fieldCondition(OG_AUDIENCE_FIELD, 'target_id', $space->id);
    }

    return $efq;
  }

   /**
   * Queries the DB for all nids in the box
   *
   * @param int $page_num
   *   Specifies a page.  FALSE to get all nids.
   * @return multitype:
   */
  public function get_nids($page_num = FALSE, &$page = NULL) {
    if (($efq = $this->get_default_query()) == FALSE) {
      return array();
    }

    $options = $this->options;

    $tagged_nodes = $this->tagged_nodes();

    //exceptions
    switch ($options['content_type']) {
      case 'book':
        //limit book depth to 1.  not a field, so use a regular query
        $query = db_select('menu_links', 'ml')
        ->condition('module', 'book')
        ->condition('depth', 1);
        $query->addField('ml', 'link_path');
        $book_nids = $query->execute()->fetchCol();
        $book_nids = preg_replace('/node\//', '', $book_nids);
        $book_nids = array_values($book_nids);

        if (count($book_nids)) {
          $efq->propertyCondition('nid', $book_nids, 'IN');
        }
        break;

      case 'biblio':
        $page = 0; //add pager properly later
        $items = $options['number_of_items'];
        $query = db_select('node', 'n')
          ->condition('n.type', 'biblio')
          ->condition('n.status', 1);

        if (module_exists('vsite') && $space = spaces_get_space()) {
          $query->join('og_membership', 'og', 'og.etid = n.nid');
          $query->condition('og.gid', $space->id);
          $query->condition('og.entity_type', 'node');
          $query->condition('og.group_type', 'node');
        }

        $query->join('biblio', 'b', 'n.vid = b.vid');

        //check biblio show all...
        if (isset($options['biblio_whitelist']) && (count(array_filter($options['biblio_whitelist'])))) {
          $bib_types = array_filter($options['biblio_whitelist']);
          $query->condition('biblio_type', array_values($bib_types), 'IN');
        }

        switch($options['sort_by']) {
          case 'sort_biblio': $query->orderBy('biblio_year', 'DESC'); $query->orderBy('n.created', 'DESC'); break;
          case 'sort_newest': $query->orderBy('n.created', 'DESC'); break;
          case 'sort_oldest': $query->orderBy('n.created', 'ASC'); break;
          case 'sort_alpha':  $query->orderBy('n.title', 'ASC'); break;
        }

        if ($page_num) {
          $query->range($page_num*$items, $items);
        }
        $query->fields('n', array('nid'));

        if (!empty($this->options['vocabs'])) {
          // If a vocabulary was selected and there were no tagged nodes then
          // we don't need to return any node.
          if (empty($tagged_nodes)) {
            $query->condition('n.nid', 0);
          }
          else {
            $query->condition('n.nid', $tagged_nodes, 'IN');
          }
        }

        $biblio_nids = $query->execute()->fetchCol();
        break;

      case 'image':
        $image_query = new EntityFieldQuery();
        $image_query->entityCondition('entity_type', 'file');
        // Add image as a condition, this will remove all media from showing up.
        $image_query->propertyCondition('type', 'image');
        if ($space = spaces_get_space()) {
          $image_query->fieldCondition(OG_AUDIENCE_FIELD, 'target_id', $space->id);
        }
        $sorts = array(
          'sort_newest' => array('timestamp', 'DESC'),
          'sort_oldest' => array('timestamp', 'ASC'),
          'sort_alpha' => array('filename', 'ASC'),
        );
        $sort = (isset($sorts[$options['sort_by']])) ? $sorts[$options['sort_by']] : $sorts['sort_newest'];
        $image_query->propertyOrderBy($sort[0], $sort[1]);

        if ($options['pager']) {
          $image_query->pager($options['number_of_items']);
          $image_page = $image_query->pager['element'];
        }
        break;
    }

    // content type
    if ($options['content_type'] != 'all') {
      $efq->propertyCondition('type', $options['content_type']);
    }

    // pager for efq
    if ($options['pager'] && $options['content_type'] != 'biblio' && is_int($page)) {
      $efq->pager($options['number_of_items']);
      $page = $efq->pager['element'];
    }
    elseif ($options['pager'] && $options['content_type'] == 'biblio' && empty($page_num)) {
      //init the pager since we don't have efq doing that for us.
      $total = count($biblio_nids);
      $limit = $options['number_of_items'];
      pager_default_initialize($total, $limit, $page);
    }

    // sort
    $sort_func = (method_exists($this, $options['sort_by'])) ? $options['sort_by'] : 'sort_newest';
    $this->$sort_func($efq);

    // display
    switch ($options['content_type']) {
      case 'biblio':
        $nids = $biblio_nids;
        break;

      case 'image':
        $result = $image_query->execute();
        $nids = isset($result['file']) ? array_keys($result['file']) : array();
        $page = (isset($image_page)) ? $image_page : NULL;
        break;

      default:
        if (!empty($this->options['vocabs'])) {
          // If a vocabulary was selected and there were no tagged nodes then
          // we don't need to return any node.
          if (empty($tagged_nodes)) {
            $efq->propertyCondition('nid', 0, '=');
          }
          else {
            $efq->propertyCondition('nid', $tagged_nodes, 'IN');
          }
        }
        $result = $efq->execute();
        $nids = isset($result['node']) ? array_keys($result['node']) : array();
        break;
    }

    return $nids;
  }

  /**
   * Return the nodes attached to terms of the selected vocabulary.
   */
  public function tagged_nodes() {
    if (empty($this->options['vocabs'])) {
      return;
    }

    // Load the taxonomy terms of the vocabulary.
    if (!$vocabulary = taxonomy_get_tree($this->options['vocabs'])) {
      return;
    }

    // Clean terms with empty value.
    if (!empty($this->options['terms'])) {
      foreach ($this->options['terms'] as $key => $value) {
        if ($value == 0) {
          unset($this->options['terms'][$key]);
        }
      }
    }

    // Collect all the taxonomy term ID.
    if (!empty($this->options['terms'])) {
      $tids = $this->options['terms'];
    }
    else {
      $tids = array();
      foreach ($vocabulary as $terms) {
        $tids[] = $terms->tid;
      }
    }

    $query = db_select('taxonomy_index', 't');
    $query->join('node', 'n', 'n.nid = t.nid');
    $result = $query
      ->condition('t.tid', $tids, 'IN')
      ->condition('n.type', $this->options['content_type'])
      ->fields('t')
      ->execute();

    $nids = array();
    foreach ($result as $row) {
      $nids[] = $row->nid;
    }

    return $nids;
  }

  /**
   * sorting functions
   */
  function sort_newest(&$efq) {
    $efq->propertyOrderBy('created', 'DESC');
  }
  function sort_oldest(&$efq) {
    $efq->propertyOrderBy('created', 'ASC');
  }
  function sort_alpha(&$efq) {
    $efq->propertyOrderBy('title', 'ASC');
  }

  function sort_event(&$efq) {
    $efq->fieldOrderBy('field_date', 'value', 'ASC');
    $efq->fieldCondition('field_date', 'value', date('Y-m-d'), '>=');
  }

  function sort_presentation(&$efq) {
    $efq->fieldOrderBy('field_presentation_date', 'value', 'DESC');
    $this->sort_newest($efq);
  }
  /**
   * display functions
   */
  function display_body($nodes) {
    $items = node_view_multiple($nodes, 'full');
    return drupal_render($items);
  }
  function display_teaser($nodes) {
    $items = node_view_multiple($nodes, 'teaser');
    return drupal_render($items);
  }
  function display_title($nodes) {
    foreach ( $nodes as $nid => $node ) {
      $items[] = l($node->title, 'node/' . $nid);
    }

    return theme('item_list', array(
      'items' => $items,
      'type' => 'ul'
    ));
  }

  function display_image($files) {
    //$html = '';
    $items = array();

    foreach ($files as $image) {
      $variables = array(
        'style_name' => "os_square_50_50",
        'path' => $image->uri,
        'height' => NULL,
        'width' => NULL,
      );
      //$html .= theme('image', $variables);
      $img = theme_image_style($variables);
      $items[] = l($img, file_create_url($image->uri), array('html'=>TRUE, 'attributes' => array('rel' => 'recent-images', 'class' => 'colorbox'))); //get a real url, hippie!
    }

    return theme('item_list', array('items'=>$items, 'attributes'=>array('class'=>array('recent-images-list'))));
  }

  function _biblio_types() {
    $rows = db_select('biblio_types', 'bt')
      ->fields('bt', array('tid', 'name'))
      ->condition('tid', 0, '>')
      ->condition('visible', 1)
      ->execute();
    $options = array('all' => 'Select All');
    foreach ($rows as $r) {
      $options[$r->tid] = $r->name;
    }

    return $options;
  }
}

/**
 * AJAX callback to add query details (e.g. the vocabulary to use).
 *
 * Since the controlling logic for populating the form is in the form builder
 * function, all we do here is select the element and return it to be updated.
 */
function os_sv_list_query_details_ajax($form, &$form_state) {
  return $form['options']['query_details'];
}

/**
 * AJAX callback for adding terms to the query.
 *
 * Since the controlling logic for populating the form is in the form builder
 * function, all we do here is select the element and return it to be updated.
 */
function os_sv_list_terms_details_ajax($form, &$form_state) {
  return $form['options']['query_details']['terms'];
}
