<?php

class SpacesOverridesBoxesMigration extends SpacesOverridesTableMigration {

  public function __construct() {
    $this->source_object = $this->dest_object = 'boxes';
    $this->dependencies = array('ModalImageNodeOS', 'SpacesOverridesContext', 'PageNodeOS', 'AnnouncementNodeOS', 'BiblioNodeOS', 'SpacesOverridesBoxesMedia'); //images have to be copied before modal boxes can migrate.
    parent::__construct();
  }

  public function query() {
    $query = parent::query();
    $query->condition('object_id', '', '!=');

    return $query;
  }


  public function prepareRow(&$row) {

    if (($value = @unserialize($row->value)) === FALSE) {
      if (strlen($row->value) > 65535) {
        //single image saved some images inline (ie img src='data:image:png....).  these were truncated in db and don't function
        parent::saveMessage(t('Box too big, source data truncated.  Ignoring.  id=@id', array('@id'=>$row->id), MigrationBase::MESSAGE_NOTICE));
      } else {
        parent::saveMessage(t('Can\'t unserialize box: id=@id, object_id=@object_id',
          array('@id' => $row->id, '@object_id' => $row->object_id)), MigrationBase::MESSAGE_WARNING);
      }
      return FALSE;
    }

    //ignore boxes we know we aren't supporting anymore.
    if (in_array($value->plugin_key, array('os_boxes_cclicense'))) {
      parent::saveMessage(t('Ignoring deprecated box: @key', array('@key'=>$value->plugin_key)), MigrationBase::MESSAGE_NOTICE);
      return FALSE;
    }

    //update default box deltas and ids
    if ($delta = SpacesOverridesContextMigration::_update_context_delta($row->object_id)) {
      //$row->object_id = $delta;
      $row->object_id = $value->delta = $delta;
    }

    if ($box = $this->_update_box($value, $row->id)) {
      $value = $box;
    } else {
      parent::saveMessage(t('Could not migrate box: @key', array('@key'=>$value->plugin_key)), MigrationBase::MESSAGE_NOTICE);
      return FALSE;
    }

    if (!$this->_box_key_exists($box->plugin_key)) {
      parent::saveMessage(t('Box @key does not exist in destination.  Ignoring box. [@box]', array('@key'=>$box->plugin_key, '@box' => print_r($box,true))), MigrationBase::MESSAGE_WARNING);
      return FALSE;
    }

//    $row->object_id = $object_id;
    $row->value = serialize($value);
    return parent::prepareRow($row);
  }

  /**
   * @see Migration::preImport()
   */
  public function preImport() {
    parent::preImport();

    $this->_copy_shield_files();
  }

  //after importing boxes, create 3 special boxes for what used to be custom themes.
  //don't worry about rollbacks.  spaces_overrides rollbacks delete entire table.
  public function postImport() {
    parent::postImport();

     $this->_fix_os_taxonomy_fbt();

    if (parent::is_domain('http://scholar.harvard.edu') && !variable_get('os_migrate_created_scholarharvard_boxes',FALSE)) {
      //#560
      $gking_html =<<<EOF
   <div class="node-inner">
<div class="os-links"></div>
<div class="content">
<ul>
	<li>How to write a publishable paper as a class project (<a
		href="/papers" rel="nofollow">HTML</a>)</li>
	<li>Advice to Junior Faculty (<a href="/files/profdev2.pdf"
		rel="nofollow">PDF</a>)</li>
	<li>Dissertation Advice (<a href="/files/diss2.pdf" rel="nofollow">PDF</a>)</li>
	<li>Wordprocessing:
	<ul>
		<li>A Quick Introduction to LaTeX (<a href="/files/lshort2.pdf"
			rel="nofollow">PDF</a>)</li>
		<li>Advanced LaTeX Commands for mathematics - Users's Guide for
		the amsmathPackage (<a
			href="ftp://ftp.ams.org/pub/tex/doc/amsmath/amsldoc.pdf"
			rel="nofollow">PDF</a>)</li>
		<li><a
			href="http://www.math.harvard.edu/computing/latex/tetex/index.html"
			rel="nofollow">TeTex Documentation Guide, advanced commands
		andpackages</a></li>
		<li>Automating bibliographies in LaTeX: use bibtex (<a
			href="http://tug.ctan.org/tex-archive/info/bibtex/tamethebeast/ttb_en2.pdf"
			rel="nofollow">PDF</a>) with Natbib (<a href="/files/natbib.pdf2"
			rel="nofollow">PDF</a>), natnotes (<a href="/files/natnotes2.pdf"
			rel="nofollow">PDF</a>), and bibentry (<a href="/files/bibentry2.pdf"
			rel="nofollow">PDF</a>)</li>
		<li>Symbols: Comprehensive List (<a
			href="http://www.ctan.org/tex-archive/info/symbols/comprehensive/symbols-a4.pdf"
			rel="nofollow">PDF</a>), <a
			href="http://detexify.kirelabs.org/classify.html" rel="nofollow">Detexify</a></li>
		<li><a href="http://ximbiot.com/cvs/wiki/" rel="nofollow">Concurrent
		Versions Systems</a></li>
		<li>Latex word counts: <a
			href="http://app.uio.no/ifi/texcount/online_latest.php"
			rel="nofollow">TeXcount</a></li>
	</ul>
	</li>
	<li>Presentations: <a href="http://latex-beamer.sourceforge.net/"
		rel="nofollow">Beamer</a>, User's Guide (<a
		href="/files/beameruserguide2.pdf" rel="nofollow">PDF</a>)</li>
	<li><a href="http://www.arl.org/sparc/author/addendum.html"
		rel="nofollow">The SPARC Author's Addendum</a> (read this if you've
	had something accepted for publication)</li>
	<li><a href="/pages/letters-recommendation" rel="nofollow">What
	to do if you want me to write you a letter of recommendation</a></li>
</ul>
</div>
</div>
EOF;

      //create new boxes
      $custom_boxes = array(
        '1' => array(
          'kshepsle_custom_box' => array(
            'region' => 'content_top',
            'body' => '<img src="/profiles/openscholar/themes/eloquent/flavors/green_modfront/shepsle-front.jpg">',
          ),
        ),
        '10057' => array(
          'agordon_custom_header_box' => array(
            'region' => 'header_second',
            'body' => '<img src="/profiles/openscholar/themes/eloquent/flavors/sand_modfront/gordonheader.gif">',
          ),
          'agordon_custom_menu_box' => array(
            'region' => 'menu_bar',
            'body' => '<ul class="os-menu"><li><a href="http://adgordon.net/biocv">Bio &amp; CV</a></li><li><a href="http://adgordon.net/publications">Publications</a></li><li><a href="http://adgordon.net/pages/teaching">Teaching</a></li><li><a href="http://adgordon.net/announcements" title="List of announcements in my site">News</a></li><li><a href="http://adgordon.net/links" title="List of links in my site">Links</a></li></ul>',
          ),
        ),
        '8' => array(
          'rbates_custom_box' => array(
            'region' => 'header_second',
            'body' => '<div class="os-rbates-map"></div>',
          ),
        ),
        '3633' => array(
          'gking_student_materials_html' => array(
            'body' => $gking_html,
            'region' => 'sidebar_second',
            'title' => 'Student Materials',
            'description' => 'Student Materials',
          ),
        ),
      );

      $new_boxes = array();
      foreach ($custom_boxes as $site => $boxes) {
        foreach ($boxes as $id => $box) {
          $new_box = array(
            'changed' => NULL,
            'delta' => $id,
            'title' => isset($box['title']) ? $box['title'] : '',
            'description' => isset($box['description']) ? $box['description'] : $id,
            'additional_classes' => '',
            'plugin_key' => 'os_boxes_html',
            'options' => array('text' => array('value'=>$box['body'], 'format'=>'filtered_html')),
            'info' => $id,
          );

          $new_boxes[] = array(
            'type' => 'og',
            'id' => $site,
            'object_type' => 'boxes',
            'object_id' => $id,
            'value' => serialize((object) $new_box),
          );
        }
      }

      $new_box = array(
        'changed' => NULL,
        'delta' => 'gking_people_fbt',
        'title' => 'By Type',
        'description' => 'Filter by People',
        'additional_classes' => '',
        'plugin_key' => 'os_taxonomy_fbt',
        'options' => array('visibility' => 'feature', 'vocabulary' => 175),
        'info' => 'gking_people_fbt',
      );

      //Gking Taxonomy
      $new_boxes[] = array(
        'type' => 'og',
        'id' => 3633,
        'object_type' => 'boxes',
        'object_id' => 'gking_people_fbt',
        'value' => serialize((object) $new_box),
      );

      $insert = db_insert('spaces_overrides')
        ->fields(array_keys($new_boxes[0]));
      foreach($new_boxes as $box) {
        $insert->values($box);
      }
      $insert->execute();
      variable_set('os_migrate_created_scholarharvard_boxes',TRUE);
    }


  }

  /**
   * @see Migration::postRollback()
   */
  public function postRollback() {
    parent::postRollback();

    $this->_remove_shield_files();

    variable_set('os_migrate_contexts_altered', FALSE);
  }

  function _update_box($box, $id) {
    $BP = new BoxPorter($this);
    $box = $BP->_update_box($box, $id);

//     if (isset($box->region)) {
//       unset($box->region);
//     }

    return $box;
  }

  function _box_key_exists($key) {
    $BP = new BoxPorter($this);
    return $BP->_box_key_exists($key);
  }

  /*
   * @function _copy_shield_files
   *
   * Copies shields from d6's vsite_shields.  Sets up varibles so they're accessible in shields box.
   */
  function _copy_shield_files() {
    //path to legacy module
    $src = variable_get('os_migrate_files', '');
    $src .= '/' . parent::d6_variable_get('file_directory_path');

    //finds the path to vsite_shields
    function find_vsite_shields_path($base, $path) {
      if ($path == 'vsite_shields') {
        return $base . '/' . $path .'/';
      }

      $new_base = "$base/$path";
      if (is_readable($new_base)) {
        foreach (scandir($new_base) as $dir) {
          if (is_dir("$new_base/$dir") && $dir != '.' && $dir !='..' ) {
            $found = find_vsite_shields_path($new_base, $dir);
            if ($found) {
              return $found;
            }
          }
        }
      }

      return FALSE;
    }

    $src = find_vsite_shields_path($src, '');

    //get files or bail out
//     if (($files = scandir($src)) === FALSE) {
//       parent::saveMessage(t('Could not read shields directory.  No shields migrated.'), MigrationBase::MESSAGE_NOTICE);
//       return FALSE;
//     }

    $dest = 'public://os_shields';
    file_prepare_directory($dest, FILE_CREATE_DIRECTORY);
    $shield_info = array();
    //$files = preg_grep('/\.png/', $files);

    //get a default
    $legacy_info = parent::d6_variable_get('vsite_shields_shield_info', FALSE);

    //add each file
    foreach ($legacy_info as $file => $info) {
      $filepath = $src . $file;
      if ($fh = @fopen($filepath, 'r')) {
        if ($saved = file_save_data($fh, $dest.'/'.$file)) {
          $shield_info[$saved->filename]['hidden'] = isset($info['hidden']) ? $info['hidden'] : FALSE;
          $shield_info[$saved->filename]['default'] = isset($info['default']) ? $info['default']: FALSE;
          $shield_info[$saved->filename]['fid'] = $saved->fid;
          if (isset($info['default']) && $info['default']) {
            variable_set('os_shields_shield', $saved->filename);
          }
        }
        fclose($fh);
      } else {
        parent::saveMessage(t('Could not open shield file: @file', array('@file'=>$file)), MigrationBase::MESSAGE_NOTICE);
      }
    }

    //and save the settings
    variable_set('os_shields_shield_info', $shield_info);
  }

  /**
   * @function _remove_shield_files
   *
   * Removes shields added by _copy_shield_files
   */
  function _remove_shield_files() {
    $shields = variable_get('os_shields_shield_info', array());
    foreach($shields as $fn => $info) {
      if ($file = file_load($info['fid'])) {
        file_delete($file, TRUE);
      }
    }

    variable_del('os_shields_shield_info');
    variable_del('os_shields_shield');
  }

  /**
   * @function _fix_os_taxonomy_fbt
   *
   * The os_taxonomy_fbt widget no longer supports selecting multiple taxonomies.
   * Widgets with multiple taxonomies selected should be split into several widgets
   */
   function _fix_os_taxonomy_fbt() {
     //We are altering the contexts, keep the context migration from overwriting our changes
     variable_set('os_migrate_contexts_altered', TRUE);

     //get the boxes that need updating
     $result = db_select('spaces_overrides', 'so')
       ->condition('type', 'og')
       ->condition('object_type', 'boxes')
       ->condition('value', '%s:10:"plugin_key";s:15:"os_taxonomy_fbt"%', 'LIKE')
       ->condition('value', '%s:10:"vocabulary";a:%', 'LIKE')
       ->fields('so')
       ->execute();

     //for each vid in a box, create a new box for that vid
     while ($row = $result->fetchObject()) {
       $value = unserialize($row->value);
       $boxes = self::split_fbt_box($row->object_id, $value);

       foreach($boxes as $object_id => $box) {
	       //Create new split box
       	 db_insert('spaces_overrides')->fields(array(
           'type' => $row->type,
           'id' => $row->id,
           'object_type' => $row->object_type,
           'object_id' => $object_id,
           'value' => serialize($box),
         ))->execute();
       }

       //and delete the original box
       db_delete('spaces_overrides')
         ->condition('type', $row->type)
         ->condition('id', $row->id)
         ->condition('object_type', $row->object_type)
         ->condition('object_id', $row->object_id)
         ->execute();

       //Update the contexts
       if(count($boxes)){
         $ct_result = db_select('spaces_overrides', 'so')
           ->condition('type', 'og')
           ->condition('id', $row->id)
           ->condition('object_type', 'context')
           ->condition('value', '%"boxes-'.$row->object_id.'"%', 'LIKE')
           ->fields('so')
           ->execute();

         while ($context = $ct_result->fetchObject()) {
         	$value = unserialize($context->value);
           	if(isset($value['blocks']['boxes-'.$row->object_id])){
           	  $old_box = $value['blocks']['boxes-'.$row->object_id];
           	  unset($value['blocks']['boxes-'.$row->object_id]);
           	  foreach ($boxes as $id => $v) {
           	  	$old_box['delta'] = $id;
           	  	$value['blocks']['boxes-'.$id] = $old_box;
           	  }
           	  db_update('spaces_overrides')
           	    ->fields(array(
                 'value' => serialize($value),
                ))
		        ->condition('type', 'og')
		        ->condition('id', $context->id)
		        ->condition('object_type', 'context')
		        ->condition('object_id', $context->object_id)
		        ->execute();
           	}else{
           	  parent::saveMessage("Unable to find box ['.'boxes-'.$row->object_id.'] in context [".print_r($context,true)."]");
           	}
         }
       }
     }
   }

   private static function split_fbt_box($object_id, $box, $plugin_only = false){
     $return = array();
   	 $vocabs = (is_array($box->options['vocabulary'])) ? array_filter($box->options['vocabulary']) : array($box->options['vocabulary']);
   	 $show_name = (isset($box->options['show_vocabulary_name'])) ? $box->options['show_vocabulary_name'] : 1;
   	 $title = $box->title;
   	 unset($box->options['vocabulary']);
   	 unset($box->options['show_vocabulary_name']);

     foreach ($vocabs as $vocab) {
     	 $id = $object_id."_fbt_".$vocab;
     	 $box->delta = $id;
     	 //Load the vocab and use its name for the box if they have that setting checked
     	 if ($show_name && !$plugin_only && count($vocabs)>1){
     	   $v = taxonomy_vocabulary_load($vocab);
     	   $box->title = $v->name;
     	 } else {
         $box->title = $title;
     	 }
     	 $box->options['vocabulary'] = $vocab;
     	 $return[$id] = clone $box;
     }

     return array_reverse($return);
   }
}

/**
 * @class BoxPorter
 *
 * Box porting functions need to be avaialble to contexts, but shouldn't be part of the rest of migration.  Lets put them all here to contain them but make them accessible.
 *
 * @author sagotsky
 *
 */
class BoxPorter {

  public function __construct($migration) {
    $this->migration = $migration;
  }

  function _update_format($format = NULL) { //no format -> plain_text
    return (in_array($format, array(0,1,2,4,5,6))) ? 'filtered_html' : 'plain_text';
  }

  public function _update_block_plugin_key($key) {

    //first see if we're dropping this box
    $unsupported = array('os_boxes_cclicense');
    if (in_array($key, $unsupported)) {
      return FALSE;
    }

    $box_map = array(
      'os_modal_tabbed' => 'os_boxes_tabs',
      'os_boxes_modal_sitelogo' => 'os_boxes_sitelogo', //needs pulling from orig vsite
      'os_boxes_modal_siteinfo' => 'os_boxes_siteinfo', //needs pulling from orig vsite
      'vsite_taxonomy_fbt' => 'os_taxonomy_fbt',
      'os_boxes_curr_toc' => 'os_boxes_booktoc', //needs an option added
      'views-os_blog_block_1' => 'views-os_blog_block',
      //'os_boxes_pub_typ_list' => 'os_boxes_pub_type',
    );


    //try mapping the key
    if (isset($box_map[$key])) {
      return $box_map[$key];
    }

    //check if we have a function for it
    if ($newkey = $this->_port_box($key, 0, TRUE)) {
      return $newkey;
    }

    //check if the key we're using exists or if we're ignoring it.
    if (module_exists('os_boxes')) {
      $boxes = os_boxes_get_boxes_plugins();
      if (isset($boxes[$key])) {
        return $key;
      } else {
        return FALSE;
      }
    } else {
      return $key;
    }

  }

  //os_sv_list_box uses function names for display types.  update them here.
  function _sv_list_box_style($style) {
    $style_map = array(
      'teasers' => 'display_teaser',
      'teaser' => 'display_teaser',
      'titles' => 'display_title',
      'title' => 'display_title',
      'full' => 'display_body',
      );

    return (isset($style_map[$style])) ? $style_map[$style] : 'display_teaser';
  }

  function _sv_list_box_sort($sort) {
    $sort_map = array(
      'a-z' => 'sort_alpha',
      'newest' => 'sort_newest',
      'oldest' => 'sort_oldest',
      );
    return (isset($sort_map[$sort])) ? $sort_map[$sort] : 'sort_newest';
  }


  public function _box_key_exists($plugin_key) {
    if (module_exists('os_boxes')) {
      $boxes = os_boxes_get_boxes_plugins();
      return isset($boxes[$plugin_key]);
    }
  }

  //refactored _update_block that also updates the box.
  public function _update_box($box, $id) {

    if (isset($box->region) && ($newregion = $this->migration->_update_region($box->region))) {
      $box->region = $newregion;
    }

    //matt says plugins should always have boxes as a module.
    if (isset($box->module)) {
      $box->module = $this->migration->_update_module($box->module);
    }

    /* three kinds of updates
     no change
    key changes
    key,box change
    box change
    delete
    */

    //first try box, box/key change.  if a function does this it should be complete and we can exit.
    $new = $this->_port_box($box, $id);
    if (isset($new)) {
      return $new; //set or false, it returns.
    }

    //key check.  either map a new key, or confirm that this key exists.
    if ($key = $this->_update_block_plugin_key($box->plugin_key)) {
      $box->plugin_key = $key;
      return $box;
    }

    //No silent failure, we don't know how to migrate this box
    $this->migration->saveMessage(t('Ignoring deprecated box: @key', array('@key'=>$box->plugin_key)), MigrationBase::MESSAGE_WARNING);
  }


  //wrapper function around the _port_box_$PLUGIN functions
  //key only is used for getting the key of the box without actually porting it.  seems easier than maintaining an array
  public function _port_box($box, $id, $key_only = FALSE) {
    if (is_string($box) && $key_only) {
      $box = (object) array('plugin_key' => $box); //make a box object for the purpose of fetching a new key.
    }

    $port = '_port_box_' . $box->plugin_key;
    if (method_exists($this, $port)) {
      return $this->$port($box, $id, $key_only);
    }

    return;
  }

  public function _port_box_os_boxes_bio($box, $id, $key_only) {
    if ($key_only) {
      return $id; //no change
    }

    if (isset($box->options['nid'])) {
      unset ($box->options['nid']);
    }

    if (!isset($box->options['teaser']) || !in_array($box->options['teaser'], array('teaser','full'))) {
      $box->options['teaser'] = 'teaser'; //does options['teaser'] even exist on this box in 6?
    }

    if (!isset($box->options['show_title'])) {
      $box->options['show_title'] = 0;
    }

    return $box;
  }

  public function _port_box_os_boxes_modal_image($box, $id, $key_only) {
    if ($key_only) {
      return 'os_boxes_html';
    }

    //get fid of new entity from migrate's status tables
    $fid = db_select('migrate_map_modalimagenodeos', 'm')
      ->condition('sourceid1', $box->options['nid'])
      ->fields('m', array('destid1'))
      ->execute()
      ->fetchCol();
    $fid = current($fid);

    //get crop data from legacy db
    $image_info = $this->migration->d6_db_select('content_type_modal_image', 'ct')
      ->condition('nid', $box->options['nid'])
      ->fields('ct', array('field_modal_image_data', 'field_link_from_image_url', 'field_modal_image_size_value'))
      ->execute()
      ->fetchAssoc();

    //get caption
    $caption = OSMigration::d6_db_select('node_revisions', 'nr')
      ->condition('nid', $box->options['nid'])
      ->fields('nr', array('body'))
      ->execute()
      ->fetchField();

    if ($fid && ($file = file_load($fid))) {
      //migrate_map_modalimagenodeos.  sourceid1 = d6 nid.  destid1 = media id.

      $data = unserialize($image_info['field_modal_image_data']);
      $image_vars = array(
        'path' => $file->uri,
        //'width' => $image_info['field_modal_image_size_value'],
        'height' => $data['crop']['height'],
        'width' => $data['crop']['width'],
        'alt' => $box->description,
        'title' => $box->title,
        'attributes' => array(),
      );

      $img_html = theme_image($image_vars);
      if (isset($image_info['field_link_from_image_url'])) {
        $img_html = l($img_html, $image_info['field_link_from_image_url'], array('html' => TRUE));
      }

      if ($caption) {
        $img_html .= $caption;
      }

      //modal image should have a separate content type migration.  lets use that for moving the image.
      $box->plugin_key = 'os_boxes_html';
      $box->options = array('text' => array(
        'format' => 'filtered_html',
        'value' => $img_html,
      ));

      return $box;
    } else {
      //in most cases of a modal_image node that didn't import, there was no group owner.
      if ($fid === FALSE) {
        $fid = 'FALSE';
      }

      //OSMigration::saveMessage
      $this->migration->saveMessage(t('Could not port os_boxes_modal_image.  No file with fid @fid.  Check legacy node: @nid',
        array('@nid' => $box->options['nid'], '@fid' => $fid)), MigrationBase::MESSAGE_WARNING);
      return FALSE;
    }
  }

  //port html box.  original was os_modal_wysiwyg
  public function _port_box_os_boxes_simple($box, $id, $key_only) {
    $key = 'os_boxes_html';
    if ($key_only) {
      return $key;
    }

    $body = $box->options['body'];
    $format = $this->_update_format($box->options['format']);

    unset($box->options['body'], $box->options['format']);

    if ($this->migration->is_domain('http://projects.iq.harvard.edu') && $box->delta == 'og-6235-4802691a') {
      $format = 'full_html'; //iqss' events this week box
    }

    $box->options = array('text' => array(
      'value' => check_markup($body, $format),
      'format' => $format,
    ));
    $box->plugin_key = $key;

    return $box;
  }

  //manual list should attach a style option
  public function _port_box_os_boxes_manual_list($box, $id, $key_only) {
    if ($key_only) {
      return $id;
    }

    if (!isset($box->options['style'])) {
      $box->options['style'] = 'title';
    }

    return $box;
  }

  public function _port_box_os_boxes_rss($box, $id, $key_only) {
    $key = 'os_boxes_rss';
    if ($key_only) {
      return $key;
    }

    if (isset($box->options['content']) && $box->options['content'] == 'all') {
      $box->options['content'] = array();
      $box->options['show_all'] = TRUE;
    } else {
      $box->options['content'] = array($box->options['content'] => $box->options['content']);
      $box->options['show_all'] = FALSE;
    }

    return $box;
  }

  //wrap other shield funciton.  apparently shield boxes had differeing names
  public function _port_box_os_boxes_shields($box, $id, $key_only) {
    $key = 'os_shields_box';
    if ($key_only) {
      return $key;
    }

    return $this->_port_box_os_shields_box($box, $id, $key_only);
  }

  //'os_boxes_shields' => 'os_shields_box', (box holds the filename.  used to be in a var.)
  public function _port_box_os_shields_box($box, $id, $key_only) {
    $key = 'os_shields_box';
    if ($key_only) {
      return $key;
    }

    //this depends on variable having already been imported.
    $value = db_select('spaces_overrides', 'so')
      ->condition('object_type', 'variable')
      ->condition('object_id', 'os_shields_shield') //name has already been transformed...
      ->condition('id', $id)
      ->condition('type', 'og')
      ->fields('so', array('value'))
      ->execute()
      ->fetchCol();

    if (is_array($value)) {
      $value = (count($value)) ? $value[0] : '' ;
    }

    if (strlen($value) && ($shield_path = @unserialize($value))) {
      $shield_file = end(explode('/', $shield_path));
      $box->options['shield'] = $shield_file;
    } else {
      $box->options['shield'] = '';
    }
    $box->plugin_key = $key;

    return $box;
  }

  //port os_boxes_oembed.  see: SpacesOverridesBoxesMediaMigration
  public function _port_box_os_boxes_oembed($box, $id, $key_only) {
    $key = 'os_boxes_media';
    if ($key_only) {
      return $key;
    }

    //get new fid from legacy delta
    $query = db_select('migrate_map_spacesoverridesboxesmedia', 'mm')
      ->condition('sourceid3', $box->delta)
      ->fields('mm', array('destid1'))
      ->isNotNull('destid1')
      ->execute();

    if ($query->rowCount()) {
      $fid = $query->fetchColumn();
      $box->options = array(
        'fid' => $fid,
        'width' => $box->options['width'],
      );
      $box->plugin_key = $key;
    } else {
      $this->migration->saveMessage(t('Could not save embed box.  No fid for box: @delta', array('@delta'=>$box->delta)), MigrationBase::MESSAGE_NOTICE);
    }

    return $box;
  }

  //recent docs list can convert to sv list
  public function _port_box_os_boxes_recent_docs($box, $id, $key_only) {
    $key = 'os_sv_list_box';
    if ($key_only) {
      return $key;
    }

    $box->options['filter'] = 'book';
    return $this->_port_box_os_boxes_sv_list($box, $id, $key_only);
  }

  //port recent pubs to sv list
  public function _port_box_os_boxes_pub_typ_list($box, $id, $key_only) {
    $key = 'os_sv_list_box';
    if ($key_only) {
      return $key;
    }

    $box->plugin_key = $key;

    //update settings.  mapping is straightforward but names have changed.
    $box->options['content_type'] = 'biblio';

    $box->options['pager'] = $box->options['use_pager'];
    unset($box->options['use_pager']);

    $box->options['sort_by'] = 'sort_biblio';

    $styles = array('titles' => 'display_title', 'teasers' => 'display_teaser');
    $box->options['display'] = (isset($styles[$box->options['style']])) ? $styles[$box->options['style']] : 'display_title';
    unset($box->options['style']);

    $box->options['biblio_show_all'] = $box->options['show_all_types'];
    unset($box->options['show_all_types']);

    $box->options['biblio_whitelist'] = $box->options['pub_types'];
    unset($box->options['pub_types']);

    return $box;
  }

  //present list should be converted to sv list
  public function _port_box_os_boxes_present_list($box, $id, $key_only) {
    $key = 'os_sv_list_box';
    if ($key_only) {
      return $key;
    }

    $box->options['filter'] = 'presentation';
    $box = $this->_port_box_os_boxes_sv_list($box, $id, $key_only);

    //if this vsite uses presentation date, update sorting to use it.
    $query = OSMigration::d6_db_select('og_ancestry', 'og')
      ->condition('og.group_nid', $id);
    $query->join('content_type_presentation', 'ct', 'ct.nid = og.nid');
    $query->fields('ct', array('field_presentation_date_value'));
    $values = $query->execute()->fetchCol();

    //do all the presentations have a date?
    $filtered = array_filter($values);
    if ($filtered && count($values) == count($filtered)) {
      $box->options['sort_by'] = 'sort_presentation';
    }

    return $box;
  }

  public function _port_box_os_boxes_sv_list($box, $id, $key_only) {
    $key = 'os_sv_list_box';
    if ($key_only) {
      return $key;
    }

    $box->plugin_key = $key;
    $ct = $box->options['filter'];
    if (strpos($ct, ':')) {
      list(,$ct) = explode(':', $ct);
    }

    //content type was updated
    $bundles = array_flip(AbstractNodeOSMigration::$source_bundle_map);
    if (isset($bundles[$ct])) {
      $ct = $bundles[$ct];
    }

    $defaults = array(
      'content_type' =>     'all',
      'display' =>          'display_title',
      'more' =>             FALSE,
      'more_link' =>        '',
      'more_text' =>        '',
      'pager' =>            FALSE,
      'number_of_items' =>  6,
      'sort_by' =>          'sort_newest',
    );

    $options = array_merge($defaults, $box->options);

    $options['content_type'] = $ct;
    $options['display'] = $this->_sv_list_box_style($options['display']);
    //some boxes used style instead of display.
    if (isset($box->options['style']) && !isset($box->options['display'])) {
      $options['display'] = $this->_sv_list_box_style($box->options['style']);
    }

    $options['sort_by'] = $this->_sv_list_box_sort($options['sort_by']);
    $options['number_of_items'] = min(20, $options['number_of_items']); //new box only goes up to 20.

    //biblio stuff
    if ($ct == 'biblio' && $options['sort_by'] == 'sort_newest') {
      $options['sort_by'] = 'sort_biblio';
    }
    $options['biblio_show_all'] = TRUE;
    $options['biblio_whitelist'] = array();


    //more link.  announcements is non-existent
    $link_update = array('announcements' => 'news', 'images' => 'galleries');
    if (isset($link_update[$box->options['more_link']])) {
      $options['more_link'] = $link_update[$box->options['more_link']];
    }

    $box->options = $options;
    return $box;
  }

  //update tabs
  public function _port_box_os_modal_tabbed($box, $plugin_key, $key_only) {
    $plugin_key = 'os_boxes_tabs';
    if ($key_only) {
      return $plugin_key;
    }

    $qt_obj = $box->options['qt_obj'];
    $new_tabs = array();
    foreach ($qt_obj['tabs'] as $id => $oldbox) {
      $new_tabs['tab-'.$id] = array(
        'bid' => SpacesOverridesContextMigration::update_context_block_id($oldbox['bid']),
        'weight' => (isset($oldbox['weight'])?$oldbox['weight']:0),
        'title' => (isset($oldbox['title'])?$oldbox['title']:""),
      );
    }

    $box->options = array(
      'additional_classes' => '',
      'tabs' => $new_tabs,
    );

    $box->plugin_key = $plugin_key;

    return $box;
  }

  //update fbt box
   public function _port_box_vsite_taxonomy_fbt($box, $id, $key_only) {
     $id = 'os_taxonomy_fbt';
     if ($key_only) {
       return $id;
     }

     //Tree
     $options['show_tree'] = 1;
     $options['tree_max_depth'] = 0;

     //Box is split in post processing $box->options['vocabulary']

     //New default and name
     $box->options['range'] = isset($box->options['number_of_items'])?$box->options['number_of_items']:false;
     $box->options['depth'] = isset($box->options['tree_max_depth'])?($box->options['tree_max_depth'] - 1):0;
     $box->options['show_childs'] = isset($box->options['show_tree'])?$box->options['show_tree']:false;
     //Don't show count by default
     $box->options['show_count'] = isset($box->options['show_count'])?$box->options['show_count']:1;

     //New Default
     if(!isset($box->options['behavior']) || $box->options['behavior'] != 'contextual'){
     	$box->options['behavior'] = '--all--';
     }

     $box->plugin_key = $id;

     //Remove old options
     unset($box->options['number_of_items']);
     unset($box->options['show_tree']);
     unset($box->options['tree_max_depth']);

     return $box;
   }

  //twitter prefix is separated into own variable
  public function _port_box_os_boxes_twitterfeed($box, $id, $key_only) {
    if ($key_only) {
      return 'os_boxes_twitterfeed';
    }

    $prefix = $box->options['twitter_username'][0];
    if ($prefix == '@') {
      $box->options['twitter_type'] = 'user';
      $box->options['twitter_username'] = substr($box->options['twitter_username'], 1);
    } elseif ($prefix == '#') {
      $box->options['twitter_type'] = 'hashtag';
      $box->options['twitter_username'] = substr($box->options['twitter_username'], 1);
    } else {
      $box->options['twitter_type'] = 'search';
    }

    if (!isset($box->options['num_items'])) {
      $box->options['num_items'] = 5;
    }

    return $box;
  }

  public function _port_box_os_boxes_modal_sitelogo($box, $id, $key_only) {
    $key = 'os_boxes_site_logo';
    if ($key_only) {
      return $key;
    }

    if (($vsite = node_load($id)) == FALSE) {
      return FALSE;
    }

    if (isset($vsite->field_site_logo[LANGUAGE_NONE]) && $logo = current($vsite->field_site_logo[LANGUAGE_NONE])) {
      $box->options = array(
        'fid' => $logo['fid'],
        'x' => $logo['cropbox_x'],
        'y' => $logo['cropbox_y'],
        'height' => $logo['cropbox_height'],
        'width' => $logo['cropbox_width'],
      );
      $box->plugin_key = $key;

      return $box;
    } else {
      $this->migration->saveMessage(t('Could not migrate site logo. Image does not exist. Vsite nid: @nid', array('@nid' => $id)), MigrationBase::MESSAGE_WARNING);
      return FALSE;
    }
  }

  //recent images -> sv list
  public function _port_box_os_boxes_gallery($box, $id, $key_only) {
    $key = 'os_sv_list_box';
    if ($key_only) {
      return $key;
    }

    $box->options['filter'] = 'image';
    return $this->_port_box_os_boxes_sv_list($box, $id, $key_only);
  }

  public function _port_box_os_boxes_slideshow($box, $id, $key_only) {
    $key = 'os_boxes_slideshow';
    if ($key_only) {
      return $key;
    }

    $defaults = array(
      'controls' => 'bottom',
      'slide_counter' => 'none',
      'show_image_titles' => 'top',
      'pager' => 'none',
      'effect' => 'fade',
      'style' => 'dark',
      'size' => 600,
    );

    $box->options = array_merge($defaults, $box->options);
    $box->options['style'] = ($box->options['style'] == '_light') ? 'light' : 'dark'; //in d6 '' => dark and dark was default as well

    $box->options['gallery_nid'] = $box->options['gallery'];
    unset($box->options['gallery']);

    $box->options['items_per_page'] = $box->options['number_of_items'];
    unset($box->options['number_of_items']);

    $box->plugin_key = $key;

    return $box;
  }

  //slideshows get the same treatment as galleries.  gain options in form.
//   public function _port_box_os_boxes_slideshow($box, $id, $key_only) {
//     return $this->_port_box_os_boxes_gallery($box, $id, $key_only);
//   }


  //convert os_boxes_node to custom html.  102 in db
  public function _port_box_os_boxes_node($box, $id,  $key_only) {
    if ($key_only) {
      return 'os_boxes_html';
    }

    $body = '';
    $box->plugin_key = 'os_boxes_html';

    $nid = $box->options['nid'];
    $teaser = $box->options['teaser'];
    $show_title = $box->options['show_title'];

    //some nids include the title.  (ie "this is my post[nid:1234]")
    if ((!(int)$nid) && strpos($nid, ':')) {
      list(,$nid) = explode(':', $nid);
      $nid = (int) $nid;
    }

    if (!$nid) {
      $this->migration->saveMessage(t('Could not port os_boxes_node.  Missing nid.'), MigrationBase::MESSAGE_WARNING);
      return FALSE; //can't port this without a nid.
    }

    //if nodes are already migrated, render new node.
    if ($node = node_load($nid)) {
      if (!$show_title){
        $node->title = '';
      }
      $build = node_view($node, $teaser);
      unset($build['#contextual_links']); //don't hardcode the admin links into the html box...

      if (isset($build['links']) && isset($build['links']['node']) && isset($build['links']['node']['#links']) && isset($build['links']['node']['#links']['node-readmore'])) {
        unset($build['links']['node']['#links']['node-readmore']); //don't add readmore text.  it wasn't in d6
      }

      //swap html back out for media tags
      $matches = array();
      $media_tag_pattern = '/\[\[.*?\]\]/';
      if (preg_match_all($media_tag_pattern, $node->body[LANGUAGE_NONE][0]['value'], $matches)) {
        $markup = $build['body'][0]['#markup'];
        foreach ($matches[0] as $media_tag) {
          $html = check_markup($media_tag, 'filtered_html');
          $markup = str_replace($html, $media_tag, $markup);
        }
        $build['body'][0]['#markup'] = $markup;
      }

      $body = drupal_render($build);
      //$format = 1; //make sure to use filtered html later

    } else {
      //if they haven't migrated, fetch the old node's body
      $query = $this->migration->d6_db_select('node', 'n')
        ->fields('n', array('nid'))
        ->condition('n.nid', $nid);
      $query->join('node_revisions', 'nr', 'n.vid = nr.vid');
      $query->fields('nr', array('title', 'body', 'teaser', 'format'));
      $result = $query->execute();

      if ($result->rowCount()) {
        $node = $result->fetchObject();

        if ($show_title) {
          $body .= $node->title;
        }

        if ($teaser = 'body') {
          $body .= $node->body;
        } else {
          $body .= $node->teaser;
        }
      }
    }

    list($body, $jc_fix) = $this->_port_box_os_boxes_node_jcarousel_fix($nid, $body);
    list($body, $biblio_fix) = $this->_port_box_os_boxes_node_biblio_fix($nid, $body);

    if ($body) {
      $box->options = array('text'=>array(
        'value' => $body,
        'format' => ($biblio_fix || $jc_fix) ? 'full_html' : 'filtered_html',
      ));

      return $box;
    } else {
      $this->migration->saveMessage(t('Could not port os_boxes_node.  No node with nid @nid.', array('@nid' => $nid)), MigrationBase::MESSAGE_WARNING);
      return FALSE;
    }
  }

  // nodes with several attached images aren't processed by jcarousel upon migration.  fix them here.
  function _port_box_os_boxes_node_jcarousel_fix($nid, $body) {
    $fixed = FALSE;

    if (strpos($body, 'class="os-files-image-list jcarousel-skin-default"')) {
      $fixed = TRUE;

      $dom = filter_dom_load($body);
      $dom->formatOutput = TRUE;
      $uls = $dom->getElementsByTagName('ul');
      for ($i=0,$len=$uls->length; $i<$len; $i++) {
        $ul = $uls->item($i);
        if ($ul->getAttribute('class') == "os-files-image-list jcarousel-skin-default") {

          //set each li's style display
          $lis = $ul->getElementsByTagName('li');
          for ($j=0,$len2=$lis->length; $j<$len2; $j++) {
            $li = $lis->item($j);
            $li->setAttribute('style', 'display:inline;');
          }

          $div = $dom->createElement('div');
          $div->setAttribute('class', 'jcarousel-skin-default');
          $child_div = $dom->createElement('div');
          $child_div->setAttribute('class', 'jcarousel-container jcarousel-container-horizontal');
          $div->appendChild($child_div);

          $ul = $ul->parentNode->replaceChild($div, $ul);
          $child_div->appendChild($ul);
        }
      }

      $body = trim($dom->saveHTML());
      $body = preg_replace('/<!DOCTYPE.*?>/', '', $body); //remove doctype
      $body = preg_replace('/.*\<body\>/m', '', $body); //remove html, body
      $body = preg_replace('/<\/body>.*/m', '', $body); //remove closing tags
      $body = trim($body);
    }

    return array($body, $fixed);
  }

  //biblio nodes get extra html that should be hidden by javascript.  remove it here instead.
  function _port_box_os_boxes_node_biblio_fix($nid, $body) {
    $fixed = FALSE;
    $type = db_select('node', 'n')
      ->condition('nid', $nid)
      ->fields('n', array('type'))
      ->execute()
      ->fetchField();

    if ($type == 'biblio') {
      //instead of changing html, tell box to use full html.  now the citaiton renders as intended, consistent with the rest of biblio
      $fixed = TRUE;
    }

    return array($body, $fixed);
  }
}
