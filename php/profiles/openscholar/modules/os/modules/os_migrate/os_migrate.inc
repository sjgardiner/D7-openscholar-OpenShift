<?php

/**
 * @class OSMigration
 *
 * Abstract class for all OpenScholar migration classes
 * Sets some defaults
 * Provides d6_db_select helper for getting d6 db for import
 */
abstract class OSMigration extends Migration {
  public function __construct() {
    // Always call the parent constructor first for basic setup
    $args = func_get_args();
    if (count($args)) {
      parent::__construct($args[0]);
    } else {
      parent::__construct();
    }

    // With migrate_ui enabled, migration pages will indicate people involved in
    // the particular migration, with their role and contact info. We default the
    // list in the shared class; it can be overridden for specific migrations.
    $this->team = array(
      new MigrateTeamMember('Jon Sagotsky', 'jsagotsky@fas.harvard.edu', t('Migration Lead')),
    );

    // Individual mappings in a migration can be linked to a ticket or issue
    // in an external tracking system. Define the URL pattern here in the shared
    // class with ':id:' representing the position of the issue number, then add
    // ->issueNumber(1234) to a mapping.
    $this->issuePattern = 'https://redmine.hmdc.harvard.edu/issues/:id:';
  }

  /**
   * @function restrict_vsites
   *
   * Returns array of nids of vsites to import.  FALSE when there's no restriction.
   *
   * Usage:
   * if ($restrict = OSMigration::restrict_vsites()) {
   *   $query->condition('id', $restrict, 'IN');
   * }
   * such that the condition's id matches vsite id for the migration class's query.
   *
   * @return boolean|Ambigous <boolean, multitype:>
   */
  static public function restrict_vsites() {
    $restrict = explode(',', variable_get('os_migrate_restrict_vsites', ''));
    $restrict = array_filter($restrict, 'is_numeric');
    return (count($restrict)) ? $restrict : FALSE;
  }

  /**
   * @function d6_db_select
   *
   * As db_select, but targets old d6 database
   * Sends users to admin page on a misconfigured db
   */
  static public function d6_db_select($target, $alias = NULL, array $options = array()) {
    static $os_migrate_database;
    if (!isset($os_migrate_database)) {
      $os_migrate_database =  variable_get('os_migrate_database', '');
    }

    try {
      return Database::getConnection('default', $os_migrate_database)
        ->select($target, $alias, $options);
    } catch (DatabaseConnectionNotDefinedException $e) {
      flood_register_event('os_migrate_bad_db');
      $msg = t('Unable to connect to legacy database %db  Please !link', array(
      				  '%db' => $os_migrate_database . '.',
      				  '!link'=>l('check settings','admin/config/openscholar/os_migrate')));
      drupal_set_message($msg, 'error');

      throw $e;
    }
  }

  public function d6_db_query ($query, array $args = array(), array $options = array()) {
    static $os_migrate_database;
    if (!isset($os_migrate_database)) {
      $os_migrate_database =  variable_get('os_migrate_database', '');
    }
    $options['target'] = $os_migrate_database;

    try {
      return Database::getConnection($options['target'], $os_migrate_database)->query($query, $args, $options);
    }  catch (DatabaseConnectionNotDefinedException $e) {
      flood_register_event('os_migrate_bad_db');
      $msg = t('Unable to connect to legacy database %db  Please !link', array(
          '%db' => $os_migrate_database . '.',
          '!link'=>l('check settings','admin/config/openscholar/os_migrate')));
      drupal_set_message($msg, 'error');

      throw $e;
    }
  }

  /**
   * @function d6_db_table_exists
   *
   * Check if table exists in legacy database.  Borrows code from db_table_exists.
   */
  function d6_db_table_exists($table) {
    static $os_migrate_database;
    if (!isset($os_migrate_database)) {
      $os_migrate_database =  variable_get('os_migrate_database', '');
    }

    try {
      return Database::getConnection('default', $os_migrate_database)->schema()->tableExists($table);
    } catch (DatabaseConnectionNotDefinedException $e) {
      flood_register_event('os_migrate_bad_db');
      $msg = t('Unable to connect to legacy database %db  Please !link', array(
          '%db' => $os_migrate_database . '.',
          '!link'=>l('check settings','admin/config/openscholar/os_migrate')));
      drupal_set_message($msg, 'error');

      throw $e;
    }
  }

  //list all content types used in d6
  function d6_list_content_types() {
    $query = self::d6_db_select('node', 'n')
      ->fields('n', array('type'))
      ->distinct();
    return $query->execute()->fetchCol();
  }
  /**
   * @function os_migrate_prepare_files
   *
   * Helper function for translating db results to array for file field.
   *
   * @param DatabaseStatementBase $result
   * 	Results from query on files table.  Requires filepath, description, list, and timestamp.
   *
   * @return array
   */

  //attaches results to row, minding the subfields (d7 field => d6 col)
  function os_migrate_prepare_files($files, &$row, $field_name, $subfields = array()) {
    //turn db statement into array of objects if that's what we're starting with.
    if (!is_array($files) && get_class($files) == 'DatabaseStatementBase') {
      $_files = array();
      while ($file_row = $files->fetchObject()) {
        $_files[] = $file_row;
      }
      $files = $_files;
    }

    //make subfields associative if it isn't.
    if (array_keys($subfields) === range(0, count($subfields)-1)) {
      $subfields = array_combine($subfields, $subfields);
    }

    //don't forget the field itself
    $subfields[''] = 'filepath';

    //create field if necessary, add results to it.
    foreach ($files as $file) {
      foreach ($subfields as $field => $column) {
        $delta = (strlen($field)) ? "${field_name}_${field}" : $field_name;

        if (!isset($row->$delta)) {
          $row->$delta = array();
        }
        //null results SHOULD be pushed.  they're placeholders.
        @array_push($row->$delta, $file->$column);
      }
    }

  }

  function get_file_by_fid($fid) {
    $result = $this->d6_db_select('files', 'f')
      ->fields('f')
      ->condition('fid', $fid, '=')
      ->execute();
    return $result->fetchObject();
  }

  /**
   * @function _migrate_filename
   *
   * @param string $filepath
   *   path to d6 file
   * @return array
   *   returns file's d7 path, name
   */
  function _migrate_filename($filepath) {
    $dir = variable_get('os_migrate_files');

    $file_directory_path = self::d6_variable_get('file_directory_path', 'sites/default/files');
    $filepath = str_replace($file_directory_path, '', $filepath);
    $file = explode('/', trim($filepath, '/'));
    $filename = array_pop($file);
    $destination_dir = implode('/', $file);

    return array($destination_dir, $filename);
  }

  /**
   * @function subfields
   *
   * Shortcut for getting an array of field:sub1, field:sub2, etc without typing field over and over.
   *
   * @param string $field
   *   Name of field
   * @param array $subfields
   *   List of subfields
   * @param string $glue
   *   Specify another string to use between field subfield.  Default=':'.
   * @return array
   */
  function subfields($field, array $subfields, $glue = ':') {
    $callback = create_function('$str', 'return "' . $field . $glue . '" . $str;');
    return array_map($callback, $subfields);
  }

  /**
   * @function os_migrate_node_get_files_query
   *
   *
   * @param int $vid - revision of node
   * @return $query - legacy db query to get list of files
   */
  function os_migrate_node_get_files_query($vid) {
    $query = self::d6_db_select('upload', 'u')
      ->fields('u', array('fid', 'vid', 'description', 'list'))
      ->condition('u.vid', $vid, '=');
    $query->join('files', 'f', 'u.fid = f.fid');
    $query->addField('f', 'filepath');
    $query->addField('f', 'timestamp');
    $query->orderBy('u.fid', 'ASC');
    return $query;
  }

  /*
   * Get variable from d6.  Cache var.
   */
  function d6_variable_get($var, $default=NULL) {
    static $variable_cache;

    if (!isset($variable_cache)) {
      $variable_cache = array();
    }

    if (!isset($variable_cache[$var])) {
      $query = self::d6_db_select('variable', 'v')
        ->fields('v', array('value'))
        ->condition('v.name', $var, '=')
        ->execute();
      if ($query->rowCount()) {
        $value = unserialize($query->fetchField());
        $variable_cache[$var] = $value;
      } else {
        $variable_cache[$var] = $default;
      }
    }

    return $variable_cache[$var];
  }

  /*
   * @function is_domain
   *
   * Checks if we're using a certain domain.  Uses purl_base_domain by default, but
   * this can be overridden with a dev var.
   *
   * Function exists for edge and special cases that may vary between installations.
   */
  function is_domain($domain) {
    if ($domain == variable_get('os_migrate_use_domain')) {
      return TRUE;
    }

    if ($domain == self::d6_variable_get('purl_base_domain')) {
      return TRUE;
    }

    return FALSE;
  }

  /*
   * Check that a file exists before letting migrate try to copy it.
   * Inline images and files have been looping when a file isn't available.
   * This helper function processes the path in the same way before trying to
   * copy so we can bail out if the file is unavailable
   */
  function legacy_file_exists($path, $log_error=TRUE) {
//  	$path = trim(parse_url($path, PHP_URL_PATH), '/');
    $path = str_replace(' ', '%20', $path);

    //try file first
    if (file_exists($path)) {
    	return TRUE;
    }

    //now try url
    $headers = @get_headers($path);
    if ($headers && ($headers[0] != 'HTTP/1.0 404 Not Found')) {
    	return TRUE;
    }

    if ($log_error) {
  	  self::saveMessage(t('Could not copy inline file %uri.  Ignoring it.', array('%uri'=>$path)), MigrationBase::MESSAGE_WARNING);
    }
    return FALSE;
  }



   /*************************
    * inline file processing
    *****************************/

    /**
     * @function os_migrate_list_inline_files
     *
     * Gets all the inline files associated with a nid.  Also checks if node bodies
     * contain inline objects that may not be in inline table
     **/
    function os_migrate_list_inline_files($vid) {
      $files = array();

      //map of tables/fields which contain references to inline media
      $table_fields = array(
        'content_field_os_inline_files' => 'field_os_inline_files_fid',
        'content_field_os_inline_image' => 'field_os_inline_image_fid',
        'content_field_os_inline_oembed' => 'field_os_inline_oembed_url',
      );

      //query each table individually.  one big query consumes too much memory.
      foreach($table_fields as $table => $field) {
        $query = $this->d6_db_select('node', 'n')
          ->condition('n.vid', $vid, '=');
        $query->join($table, 't', 'n.vid = t.vid');
        $query->fields('t', array($field));
        $query->isNotNull($field);

        foreach ($query->execute() as $row) {
          $files[] = array_filter((array)$row);
        }
      }

      //and check the node bodies (done separately since condition is more complicated

      $query = $this->d6_db_select('node', 'n')
        ->condition('n.vid', $vid, '=');
      $query->join('node_revisions', 'nr', 'n.vid = nr.vid');
      $query->fields('nr', array('body'));
      $query->condition(db_or()
        ->condition('nr.body', '%<iframe%', 'LIKE')
        ->condition('nr.body', '%<object%', 'LIKE')
        ->condition('nr.body', '%<embed%', 'LIKE')
      );

      foreach ($query->execute() as $row) {
        $arr = array_filter((array)$row);
        if (!count($arr)) { $arr['body'] = TRUE; }
        $files[] = $arr;
      }

    return $files;
  }

  /**
   * @function os_migrate_update_body_files(&$row, $fid)
   *
   * Given a node's row and the fid of an inline file in that node, update the body text to
   * refer to the new file entity
   **/
  function os_migrate_update_body_files(&$row, $fid) {
    $query = db_select('migrate_map_fileinlineos', 'm')
      ->fields('m', array('destid1'))
      ->condition('sourceid1', $fid, '=')
      ->isNotNull(('destid1'));
    $result = $query->execute();
    $file = $result->fetchAssoc();
    if (!$file) {
      return;
    }


    //get original name, for matching the url.  if files table isn't cleared between migrations, aliases can pile up and block name matching
    $d6name = self::d6_db_select('files', 'f')
      ->condition('fid', $fid)
      ->fields('f', array('filename'))
      ->execute()
      ->fetchField();

    if (($entity = file_load($file['destid1'])) == FALSE) {
      self::saveMessage(t('Error loading file: @fid', array('@fid'=>$file['destid1'])), MigrationBase::MESSAGE_WARNING);
      return FALSE;
    }
    $uri = ($d6name) ? $d6name : $this->_uri_trim($entity->uri);

    $dom = filter_dom_load($row->body);
    $links = $dom->getElementsByTagName('a');
    $update = FALSE;
    for ($i=0,$len=$links->length; $i<$len; $i++) {
      $link = $links->item($i);
      if ($link && strpos($link->getAttribute('href'), $uri)) {
        $update = TRUE;
        $tag = $this->_os_migrate_inline_tag('file', $entity->fid, $link);
        $text = $dom->createTextNode($tag);
        $link->parentNode->replaceChild($text, $link);
      }
    }

    if ($update) {
      $row->body = $this->_os_migrate_update_body_clean($dom->saveHTML());
    }
  }

  /**
   * @function os_migrate_update_body_url(&$row, $url)
   *
   * Given a node's row and the url of an inline link in that node, update the body text to
   * refer to the new file entity
   **/
  function os_migrate_update_body_url(&$row, $url) {
    //get this url's delta.
    $delta = self::d6_db_select('content_field_os_inline_oembed', 'cf')
      ->condition('field_os_inline_oembed_url', $url)
      ->condition('vid', $row->vid)
      ->fields('cf', array('delta'))
      ->execute()
      ->fetchColumn();

    $query = db_select('migrate_map_embedinlineos', 'm')
      ->fields('m', array('destid1'))
      ->condition('sourceid2', $delta)
      ->condition('sourceid1', $row->vid, '=');

    $entity_id = $query->execute()->fetchAssoc(); //only the one?
    if (!$entity_id) {
      parent::saveMessage(t('Could not update url in node body.  Missing file entity. @nid', array('@nid'=>$row->nid)), MigrationBase::MESSAGE_NOTICE);
      return FALSE;
    }

    $entity = file_load($entity_id['destid1']);
    if (!$entity) {
      parent::saveMessage(t('Could not load migrate inline file: @fid', array('@fid'=>$entity_id['destid1'])), MigrationBase::MESSAGE_NOTICE);
      return FALSE;
    }


    $dom = filter_dom_load($row->body);
    //check all a tags.  we'll have to replace parent though since its a space for the oembed
    $anchors = $dom->getElementsByTagName('a');
    $update = FALSE;
//     for ($i=0, $len=$anchors->length; $i<$len; $i++) {
//       $a = $anchors->item($i);
//       if (isset($a) && ($a->getAttribute('href') == $url)) {
//         $tag = $this->_os_migrate_inline_tag('url', $entity->fid, $a);
//         $text = $dom->createTextNode($tag);
//         $span = $a->parentNode;
//         $span->parentNode->replaceChild($text, $span);

//         $update = TRUE;
//       }
//     }

    //html didn't work.  try media tags.
    if ($update == FALSE && isset($entity->fid)) {
      $divs = $dom->getElementsByTagName('div');
      for ($i=0, $len=$divs->length; $i<$len && !$update; $i++ ) {
        $div = $divs->item($i);
        if (preg_match('/^\[wysiwyg_field.*?\]/', trim($div->nodeValue))) {
          if ($tag = $this->_os_migrate_inline_port_tag($div->nodeValue, $entity->fid, $row->vid)) {
            $text = $dom->createTextNode($tag);
            $div->parentNode->replaceChild($text, $div);
            $update = TRUE;
          }
        }
      }
    }

    //save changes
    if ($update) {
      $row->body = $this->_os_migrate_update_body_clean($dom->saveHTML());
    }
  }

  /**
   * @function os_migrate_update_body_body(&$row, $fid)
   *
   * Given a node's row and the fid of a file extracted from the body of the node, update the body text to
   * refer to the new file entity
   **/
  function os_migrate_update_body_body(&$row, $fid) {
    $query = db_select('os_migrate_body_inline_objects', 'o')
      ->fields('o', array('html'))
      ->condition('vid', $row->vid, '=');
    $query->join('migrate_map_htmlinlineos', 'm', 'o.id = m.sourceid1');
    $query->fields('m', array('destid1'));
    $query->isNotNull('m.destid1');

    $data = $query->execute()->fetchAssoc();
    $update = FALSE;
    $dom = filter_dom_load($row->body);

    //lets try iframes first
    $target_dom = filter_dom_load($data['html']);
    $target_iframes = $target_dom->getElementsByTagName('iframe');
    if ($target_iframes->length) {
      $target_src = $target_iframes->item(0)->getAttribute('src');

      $iframes = $dom->getElementsByTagName('iframe');
      for ($i=0,$len=$iframes->length; $i<$len; $i++) {
        $iframe = $iframes->item($i);
        if ($iframe && ($src = $iframe->getAttribute('src'))) {
          if ($src == $target_src) {
            $update = TRUE;
            $tag = $this->_os_migrate_inline_tag('iframe', $data['destid1'], $iframe);
            $text = $dom->createTextNode($tag);
            $iframe->parentNode->replaceChild($text, $iframe);
          }
        }
      }
    }

    //and objects.  some have data.  others have children embeds.
    $target_objects = $target_dom->getElementsByTagName('object');
    if ($target_objects->length) {
      $target_object = $target_objects->item(0);
      $target_data = $target_object->getAttribute('data');
      $target_embed = $target_object->getElementsByTagName('embed');
      if ($target_embed->length) {
        $target_embed_src = $target_embed->item(0)->getAttribute('src');
      }

      $objects = $dom->getElementsByTagName('object');
      for ($i=0,$len=$objects->length; $i<$len; $i++) {
        $object = $objects->item($i);
        if ($object && ($src = $object->getAttribute('data'))) {
          //data objects - currently untested.
          if ($target_data == $src) {
            $update = TRUE;
            $tag = $this->_os_migrate_inline_tag('object-data', $data['destid1'], $object);
            $text = $dom->createTextNode($tag);
            $object->parentNode->replaceChild($text, $object);

          } elseif ($object && ($embed = $object->getElementsByTagName('embed'))) {
            //embed objects
            if ($target_embed_src == $embed->item(0)->getAttribute('src')) {
              $update = TRUE;
              //can't convince wysiwyg to take these strings.  lets ignore that for now and update tag later.
              $tag = $this->_os_migrate_inline_tag('object-embed', $data['destid1'], $object);
              $text = $dom->createTextNode($tag);
              $object->parentNode->replaceChild($text, $object);
            }
          }
        }
      }
    }

    $target_embeds = $target_dom->getElementsByTagName('embed');
    $embeds = $dom->getElementsByTagName('embed');

    for ($i=0,$len=$embeds->length; $i<$len; $i++) {
      $embed = $embeds->item($i);
      if ($embed && ($embed_src = $embed->getAttribute('src'))) {

        for ($j=0,$len=$target_embeds->length; $i<$len; $i++) {
          $target_embed = $target_embeds->item($j);
          if ($target_embed && ($embed_src = $target_embed->getAttribute('src'))) {
            $tag = $this->_os_migrate_inline_tag('embed', $data['destid1'], $embed);
            $text = $dom->createTextNode($tag);
            $embed->parentNode->replaceChild($text, $embed);
            $update = TRUE;
            break;
          }
        }
      }
    }


    if ($update) {
      $row->body = $this->_os_migrate_update_body_clean($dom->saveHTML());
    }

    return;
  }

  /**
   * @function os_migrate_update_body_image(&$row, $fid)
   *
   * Given a node's row and the fid of an inline image in that node, update the body text to
   * refer to the new file entity
   **/
  function os_migrate_update_body_image(&$row, $fid) {
    $query = db_select('migrate_map_imageinlineos', 'm')
      ->fields('m', array('destid1'))
      ->condition('sourceid1', $fid, '=')
      ->isNotNull(('destid1'));
    $entity_id = $query->execute()->fetchAssoc();
    if (! ($entity_id && ($entity = file_load($entity_id['destid1'])))) {
      self::saveMessage(t('Migration error on source file %fid.  Could not find image file in migration tables.  Continuing without file.  Check Inline migrations.', array('%fid'=>$fid)), MigrationBase::MESSAGE_WARNING);
      return;
    }

    //get original name, for matching the url.  if files table isn't cleared between migrations, aliases can pile up and block name matching
    $d6name = self::d6_db_select('files', 'f')
      ->condition('fid', $fid)
      ->fields('f', array('filename'))
      ->execute()
      ->fetchField();

    //replace this with generic function.  does the one I wrote make sense?  wtf is this one doing?
    //were we stripping just the purl for the site?
    $uri = (isset($d6name) && $d6name) ? $d6name : self::_uri_trim($entity->uri);

    $dom = filter_dom_load($row->body);
    $images = $dom->getElementsByTagName('img');
    $update = FALSE;
    for ($i=0, $len=$images->length; $i < $len; $i++) {
      $img = $images->item($i);
      if (isset($img) && strpos($img->getAttribute('src'), $uri)) {
        $src = $img->getAttribute('src');
        $class = $img->getAttribute('class');

        $imagecache_dims = array();
        if (preg_match('/^imagecache/', $class) || strpos($src, 'imagecache')) {
          $imagecache_dims = $this->_get_inline_imagecache_dimensions($src);
        }

        if (($width = $img->getAttribute('width')) && $height = $img->getAttribute('height')) {
          $imagecache_dims['width'] = $width;
          $imagecache_dims['height'] = $height;
        }

        $style = $this->_get_image_attributes($img);
        $parent_style = $this->_get_image_attributes($img->parentNode);
        $style = array_merge($parent_style, $style);

        if ($imagecache_dims) {
          $style[] = 'width: ' . $imagecache_dims['width'];
          $style[] = 'height: ' . $imagecache_dims['height'];
        }

        $attrs = array();
        foreach(array('width', 'height', 'class') as $attr) {
          if (isset($$attr) && $$attr) {
            $attrs[$attr] = $$attr;
          }
        }

        $tag = $this->_os_migrate_inline_tag('image', $entity->fid, $img, $style, $imagecache_dims, $attrs);
        $text = $dom->createTextNode($tag);
        $img->parentNode->replaceChild($text, $img);
        $update = TRUE;
      }
    }

    if ($update) {
      $row->body = $this->_os_migrate_update_body_clean($dom->saveHTML());
    }

  }

  /**
   * @function os_migrate_inline_update
   *
   * Check node row for inline embeds.  If any exist update body to use new file entities.
   **/
  function os_migrate_inline_update(&$current_row) {
    $files = $this->os_migrate_list_inline_files($current_row->vid);
    if (!count($files)) {
      return;
    }

    $file_funcs = array(
      'field_os_inline_image_fid' => 'image',
      'field_os_inline_files_fid' => 'files',
    	'field_os_inline_oembed_url' => 'url',
    	'body' => 'body',
    );

    foreach ($files as $file) {
      foreach ($file as $src => $fid) {
        $func = 'os_migrate_update_body_' . $file_funcs[$src];
        $this->$func($current_row, $fid);
      }
    }
    return;
  }

  //prepare the inline tag
  //generic for now.
  function _os_migrate_inline_tag($type, $fid, &$dom_object, $style = array(), $attributes=array()) {
    $tag = array(
      'type' => 'media',
      'view_mode' => 'default',
      'fid' => $fid,
      'attributes' => array(
        'class' => 'media-image',
        'typeof' => 'foaf:Image',
      ),
    );


    //os_files_ large=600px, xlarge=800px, xxlarge=1000px
    $sizes = array(
      'imagecache-inline_larger' => 'os_files_large',
      'imagecache-inline_default' => 'os_files_large',
      '' => 'os_files_xlarge', //default when no class is present
      'os-fullbanner' => 'os_files_xxlarge',
      'os-halfbanner' => 'os_files_xlarge',
    );



    switch ($type) {
      case 'image':
        //images may have titles but there's no way to enter a title in media
        $class = $dom_object->getAttribute('class');
        if (isset($sizes[$class])) {
          $tag['view_mode'] = $sizes[$class];
        } else {
          $tag['view_mode'] = $sizes[''];
          parent::saveMessage(t('No image cache size for class: @class, assuming original.', array('@class' => $class)), MigrationBase::MESSAGE_INFORMATIONAL);
        }

        $attributes = (empty($attributes)) ? array('width'=>NULL, 'height'=>NULL) : $attributes;
        //$tag['view_mode'] = 'media_original';
      break;

      case 'file':
      break;

      case 'url':
      break;

      //<p>[[{"type":"media","view_mode":"default","fid":"1234","attributes":{"class":"file-icon media-image"}}]]</p>
      case 'iframe':
//        $attributes = array('width'=>'width', 'height'=>'height');
        //style attr?  keep or ditch
      break;
    }

    $tag['attributes'] += $this->_os_migrate_get_attributes($attributes, $dom_object);
    if ($style) {
      $tag['attributes']['style'] = implode('; ', $style);
    }
    return ' [[' . drupal_json_encode($tag) . ']] ';
  }

  /**
   * @function _os_migrate_inline_port_tag()
   *
   * Updates a d6 media tag for d7.
   *
   * @param str $tag
   *   Text of tag.
   * @param int $fid
   *   ID of migrated file
   * @param int $nid
   *   Node id
   *
   * @return string
   *   Media 7.x tag
   */
  function _os_migrate_inline_port_tag($tag, $fid, $vid) {
    //make an array of the tag.
    $tag = trim($tag, '[]');
    $info = array();
    foreach (explode(' ', $tag) as $pair) {
      $pair = explode('=', $pair);
      $info[$pair[0]] = (isset($pair[1])) ? trim($pair[1], '"') : NULL;
    }

    //check that this tag matches the embedded file being attached to it
    $result = db_select('migrate_map_embedinlineos', 'mm')
      ->condition('sourceid1', $vid)
      ->condition('sourceid2', $info['wf_deltas'])
      ->condition('destid1', $fid)
      ->fields('mm', array('destid1'))
      ->execute();
    if (!$result->rowCount()) {
      return FALSE;
    }

    //right file, build the tag
    $tag = array(
      'type' => 'media',
      'view_mode' => 'default',
      'fid' => $fid,
      'attributes' => array(
        'class' => 'media-image',
        //'typeof' => 'foaf:Image',
        //no idea where width and height are coming from
      ),
    );

    return ' [[' . drupal_json_encode($tag) . ']] ';
  }

  /**
   * @fucntion _get_image_attributes
   *
   * Inline image tags take css to apply to the images upon rendering.  This function
   * extracts the desired css elements from a dom element and returns an array
   * of strings.
   */
  function _get_image_attributes($elem) {
    $styles = $elem->getAttribute('style');
    $styles = explode(';', $styles);
    $css = array();
    foreach (array_filter($styles) as $style) {
      $t = explode(':', $style);
      $css[trim($t[0])] = trim($t[1]);
    }

    //just getting floats for now.  that may change later.
    $css = array_intersect_key($css, array_flip(array('float', 'text-align')));

    $return = array();
    foreach ($css as $prop => $val) {
//       if ($prop == 'text-align' && $val = 'center') {
//         $prop = 'margin';
//         $val = '0 auto';
//         $return[] = 'display: block';
//       }
      $return[] = $prop .': '. $val;
    }

    return $return;
  }

  /**
   * @function _get_inline_imagecache_dimensions($uri)
   *
   * Some inline images don't have their dimensions stored anywhere, but leave a cropped
   * file in an imagecache folder.  In that case, return the [width, height] of the
   * image.
   *
   * ex URI: http://scholar.harvard.edu/swallow/sites/scholar.iq.harvard.edu/files/imagecache/inline_default/swallow/files/polar-bear-wild-animals-2614076-1024-768.jpg
   */
  function _get_inline_imagecache_dimensions($uri) {
//    $domain = variable_get('os_migrate_use_domain', self::d6_variable_get('purl_base_domain', FALSE));

    $files_path = variable_get('os_migrate_files');
    $drupal_files = self::d6_variable_get('file_directory_path', 'sites/default/files');
  //  $file = preg_replace('/.*\/files/U', '', $uri);
    $pattern = '/.*' . str_replace('/', '\/', $drupal_files) .'/';
    $file = preg_replace($pattern, '', $uri);

    $filepath = "${files_path}/${drupal_files}/${file}";

    if (is_readable($filepath) && ($dims = getimagesize($filepath))) {
      return array('width'=>$dims[0], 'height'=>$dims[1]);
    } else {
      self::saveMessage(t('Could not find dimensions for image: %uri', array('%uri'=>$uri)), MigrationBase::MESSAGE_INFORMATIONAL);
      return FALSE;
    }


  }
  /**
   * get attributes from dom element
   * attr array maps old (d6) to new d7 attribute, just in case things change due to new schema
   **/
  function _os_migrate_get_attributes(array $attr, DOMElement $dom) {
    $return = array();
    foreach ($attr as  $attr => $value) {
      if (empty($value)) {
        $value = $dom->getAttribute($attr);
      }
      $return[$attr] = $value;
    }
    return $return;
  }

  //body saved by dom is full document.  strip doctype, html, body tags.
  function _os_migrate_update_body_clean($body) {
    $body = preg_replace('/<!DOCTYPE.*?>/', '', $body); //remove doctype
    $body = preg_replace('/.*\<body\>/m', '', $body); //remove html, body
    $body = preg_replace('/<\/body>.*/m', '', $body); //remove closing tags
    return trim($body);
  }

  //remove public:// or whatever :// appears before uri
  function _uri_trim($uri) {
    $pattern = '/.*?:\/\//';
    if (preg_match($pattern, $uri)) {
      $uri = preg_replace($pattern, '', $uri);
    }
    return $uri;
  }

  //file entities have no field for og_group_ref.  update file_usage
  //to let drupal know which node they're attached to
  function _os_migrate_inline_files_update_file_usage($entity, $row) {
    if ($entity->fid && $row->nid) {
      file_usage_add($entity, 'os_wysiwyg', 'node', $row->nid);
    } else {
      return FALSE; //does this ever happen?  nid==0
    }
  }

  /**
   * @function _get_destination_path
   *
   * Updates the destination path a file will be saved to.  Tries to figure out purl given a d6 entity
   *
   * @param string $type
   *   What type of entity does the id refer to (file, node, etc)
   * @param int $id
   *   Id of legacy object.
   *
   */
  public static function _get_destination_path($type, $id) {
    $cache = drupal_static(__FUNCTION__, array());
    $key = $type . '::' . $id;
    if (isset($cache[$key])) {
      return $cache[$key];
    }

    //given a type, make a query that joins purl on the group
    switch ($type) {
       case 'inline_image':
        $query = self::d6_db_select('content_field_os_inline_image', 'ct')
          ->condition('ct.field_os_inline_image_fid', $id);
        $query->join('node', 'n', 'ct.nid = n.nid');
        $query->condition('n.nid', $id);
        $query->join('og_ancestry', 'oga', 'n.nid = oga.nid');
        $query->join('purl', 'p', 'p.id = oga.group_nid');
        break;

      case 'inline_file':
        $query = self::d6_db_select('content_field_os_inline_files', 'ct')
          ->condition('ct.field_os_inline_files_fid', $id);
        $query->join('node', 'n', 'ct.nid = n.nid');
        $query->condition('n.nid', $id);
        $query->join('og_ancestry', 'oga', 'n.nid = oga.nid');
        $query->join('purl', 'p', 'p.id = oga.group_nid');
        break;

      case 'node':
        $query = self::d6_db_select('node', 'n')
          ->condition('n.nid', $id);
        $query->join('og_ancestry', 'oga', 'n.nid = oga.nid');
        $query->join('purl', 'p', 'p.id = oga.group_nid');
        break;

      case 'file':
        $query = self::d6_db_select('upload', 'u')
          ->condition('u.fid', $id);
        //lets start with upload.  might have to add an additional tables param if this isn't enoug hfor inline

        $query->join('node', 'n', 'u.nid = n.nid');
        $query->join('og_ancestry', 'oga', 'n.nid = oga.nid');
        $query->join('purl', 'p', 'p.id = oga.group_nid');
        break;

      case 'og':
        $query = self::d6_db_select('purl', 'p')
          ->condition('id', $id);
        break;

      default:
        $cache[$key] = FALSE;
        return FALSE;
    }

    //now that we have node, getting the purl value is easy.
    $query->condition('p.provider', 'spaces_og');
    $query->fields('p', array('value'));
    $result = $query->execute();

    if ($result->rowCount()) {
      $purl = $result->fetchField();
      $return = ($purl) ? 'public://' . $purl . '/files' : 'public://';
    } else {
      $return = FALSE;
    }

    $cache[$key] = $return;
    return $return;
  }
}


