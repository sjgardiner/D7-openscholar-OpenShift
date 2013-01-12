<?php

/**
 * @file
 *
 * Abstract migration class for nodes
 **/

/**
 * Abstract node migration class
 *
 * Defines mappings for node table.  Let each type implement this and dscribe the specifics
 */
abstract class AbstractNodeOSMigration extends OSMigration {

  //Map of bundle names that have changed.  d7 bundle => d6 cck type
  public static $source_bundle_map = array(
    'software_project' => 'scholar_software_project',
    'software_release' => 'scholar_software_release',
    'media_gallery' => 'gallery',
    'news' => 'announcement',
  );

  private $comment_maintain_node_statistics;

  public function __construct($destination_bundle){
    $this->bundle = $destination_bundle;

    $this->source_content_type = $source_content_type = self::get_source_bundle($destination_bundle);

    parent::__construct(MigrateGroup::getInstance('node', array('inline','users')));
    //when extending this class, don't forget to update info file with new filename!


    // if desired, supply node with defaults.
    $this->source = $this->source($source_content_type); //condtion type?
    $this->destination = $this->destination($destination_bundle);
    $this->map = $this->map($source_content_type);
    $this->dependencies = array(
      'UsersOS',
      'EmbedInlineOS',
      'ImageInlineOS',
      'FileInlineOS',
      'HTMLInlineOS',
      'VsiteNodeOS',
    ); //og when that's done


    /* mappings */
    $this->addSimpleMappings(array(
      'created',
      'changed',
      'status',
      'promote',
      'sticky',
      'comment',
      'title',
      'tnid',
      'uid',
      'nid'
    ));

    $this->addFieldMapping('language', 'language')->defaultValue('und');
    $this->addFieldMapping('is_new')->defaultValue(TRUE);
    $this->addFieldMapping('revision_uid', 'nr_uid');

    $this->addFieldMapping('body', 'body');
    $this->addFieldMapping('body:format', 'format')->defaultValue('filtered_html');
    $this->addFieldMapping('body:language', 'language')->defaultValue('und');
  //  $this->addFieldMapping('body:summary', 'teaser');

    $this->addFieldMapping('log', 'log'); //from nr
    $this->addFieldMapping('revision')->defaultValue(TRUE); //create a new revision.  vid is automatically migrated
    $this->addFieldMapping('path', 'path');
    $this->addFieldMapping('pathauto')->defaultValue(FALSE); //do not create new paths.  paths already exist.


    $this->_map_file_field();

    //dev setting: force nodes to be in a set group
    $force_og = variable_get('os_migrate_force_og', 0);
    if (! $force_og) {
      $this->addFieldMapping('og_group_ref', 'group_nid');
    }
    else {
      $this->addFieldMapping('og_group_ref')->defaultValue($force_og);
      $this->addUnmigratedSources(array(
        'group_nid'
      ), t('DNM'));
    }

    $this->addUnmigratedDestinations(array('body:summary'), t('DNM'));
    /* Unmapped source fields.  Mark them as Do Not Map, so the UI knows they're intentionally ignored */
    $this->addUnmigratedSources(array(
      'moderate',
      'translate',
      'type',
      'format_name',
      'vid'
    ), t('DNM'));
  }

  /**
   * several helper functions for setting up child nodes.
   * it's not clear to me yet what level of abstraction I'll be working at
   * so for now I'm leaivng it up to the node that implements this class
   * to use the provided options or come up with their own.
   */
  public function query($content_type){
    $query = parent::d6_db_select('node', 'n')->fields('n', $this->getNodeFields())->condition('n.type', $content_type, '=');

    //nodes to test files with:
    //$query->condition('n.nid', '20301', '='); //prod node with uploads
    //$query->condition('n.nid', '6811', '='); //prod node with inline files
    //$query->condition('n.nid', '13701', '='); //prod node with multiple images

    $query->join('node_revisions', 'nr', 'n.vid = nr.vid');
    $query->fields('nr', array(
      'body',
      'uid',
      'log',
      //'teaser'
    ));

    $query->leftjoin('filter_formats', 'ff', 'nr.format = ff.format');
    $query->addField('ff', 'name', 'format_name');
    $query->orderBy('n.changed');

    $query->join('og_ancestry', 'og', 'n.nid = og.nid');
    $query->fields('og', array(
      'group_nid'
    ));
    $query->condition('og.group_nid', '0', '!=');

    $table = 'content_type_' . $content_type;
    if (parent::d6_db_table_exists($table)) {
      $query->leftjoin($table, 'ct', 'n.vid = ct.vid');
      $query->fields('ct');
    }

    if ($restrict = OSMigration::restrict_vsites()) {
      $query->condition('og.group_nid', $restrict, 'IN'); //dancabral, jlepore, jsagotsky, gary
    }

    return $query;
  }

  private function getNodeFields(){
    return array(
      'nid',
      'vid',
      'type',
      'language',
      'title',
      'uid',
      'status',
      'created',
      'changed',
      'comment',
      'promote',
      'moderate',
      'sticky',
      'tnid',
      'translate'
    );
  }

  public function sourceFields($content_type){
    $source_fields = array(
      'nid' => t('Node ID'),
      'path' => t('URL Path Alias'),
      'format' => t('Text format.  Usually filtered html.')
    );

    //get columns front content_type_$content_type
    $table = 'content_type_' . $content_type;
    if (parent::d6_db_table_exists($table)) {
      $result = parent::d6_db_query('DESCRIBE ' . $table);
      while ( $row = $result->fetchAssoc() ) {
        $field = $row['Field'];
        if (! in_array($field, array(
          'nid',
          'vid'
        ))) {
          $source_fields[$field] = t('(From node abstract) @field', array(
            '@field' => $field
          ));
        }
      }
    }

    $source_fields += $this->_source_fields_field_upload();

    //    $source_fields += parent::sourceFieldsCCK();
    return $source_fields;
  }

  public function source($content_type){
    return new MigrateSourceSQL($this->query($content_type), $this->sourceFields($content_type), null, array(
      'map_joinable' => FALSE
    ));
  }

  public function destination($bundle){
    return new MigrateDestinationNode($bundle);
  }

  //map provides the schema for the primary key of the source table.  this lets migrate associate a unique source id
  //with a migrated object
  public function map($content_type){
    return new MigrateSQLMap($this->machineName, array(
      'nid' => array(
        'type' => 'int',
        'not null' => TRUE,
        'unsigned' => TRUE,
        'description' => 'D6 ' . $content_type . ' ID.',
        'alias' => 'n'
      )
    ), MigrateDestinationNode::getKeySchema());
  }

  /**
   * Implements prepareRow
   *
   * Fetches path from {url_alias}
   */
  function prepareRow(&$current_row){

    //check that destination vsite exists.  some vsites have been removed, but their content still exists
    $query = db_select('node', 'n')->fields('n')->condition('type', $this->_d7_vsite_type())->condition('nid', $current_row->group_nid, '=');

    if (! $query->execute()->fetchAssoc()) {
      $os_migrate_legacy_site = variable_get('os_migrate_legacy_site', '');
      if (strlen($os_migrate_legacy_site)) {
        $blank = array(
          'attributes' => array(
            'target' => '_blank'
          )
        );
        Migration::saveMessage(t('Vsite !gid does not exist.  Assuming node !nid is orphaned.', array(
          '!gid' => l($current_row->group_nid, $os_migrate_legacy_site . '/node/' . $current_row->group_nid, $blank),
          '!nid' => l($current_row->nid, $os_migrate_legacy_site . '/node/' . $current_row->nid, $blank)
        )), Migration::MESSAGE_NOTICE);
      }
      else {
        Migration::saveMessage(t('Vsite @gid does not exist.  Assuming node @nid is orphaned.', array(
          '@gid' => $current_row->group_nid,
          '@nid' => $current_row->nid
        )), Migration::MESSAGE_NOTICE);
      }
      return FALSE;
    }

    //check if bundle exists.
    $info = field_info_bundles('node');
    if (! isset($info[$this->bundle])) {
      Migration::saveMessage(t('Could not migrate into non-existant bundle: @bundle', array(
        '@bundle' => $this->bundle
      )));
      return FALSE;
    }

    //get path from {url_alias}
    $query = parent::d6_db_select('url_alias', 'ua')->fields('ua', array(
      'dst'
    ))->condition('ua.src', 'node/' . $current_row->nid, '=');
    $results = $query->execute()->fetchAssoc();
    $current_row->path = $results['dst'];

    //lanuage should be explicitly undefined instead of blank
    if (! strlen($current_row->language)) {
      $current_row->language = 'und';
    }

    //format.  legacy formats are Filtered HTML, Full HTML, PHP Code, Comments HTML, Minimal WYSIWYG, FUll WYSWIWYG
    switch ($current_row->format_name) {
      case 'PHP Code' :
        $current_row->format = 'plain_text';
        break;
      default :
        $current_row->format = 'filtered_html';
    }

    //adjust the body to use new fids for inline objects
    parent::os_migrate_inline_update($current_row);

    $this->_prepare_file_field($current_row);

    return parent::prepareRow($current_row);
  }

  /*
   * Implements complete()
   *
   * files with list=0 should be copied, but should not show up as attachments.
   * Instead of letting the field decide if they're attached, we're going to remove
   * their connection to the node, leaving the file in the filesystem.
   */
  public function complete($entity, $row) {
    if (!isset($row->field_upload) || empty($row->field_upload)) {
      return;
    }

    $node_dirty = FALSE;

    //update links to attached files.  they should have been updated to include files/$purl
    $node_dirty = (self::_complete_body_links($entity, $row) || $node_dirty);

    foreach ($entity->field_upload as $lang => $fields) {
      foreach ($fields as $delta => $upload) {
        $file_dirty = FALSE;
        $file = file_load($upload['fid']);


        //update display
        if ($row->field_upload_display[$delta] == '0') {
          //file_usage_add(file_load($upload['fid']), 'file', 'node', $entity->nid);
          file_usage_delete($file, 'file', 'node', $entity->nid, 0); //delete the reference from this node to file, but don't decrease its count.
          unset($entity->field_upload[$lang][$delta]);
          $node_dirty = TRUE;
        }

        //update title
        if ($file && $row->field_upload_description[$delta]) {
          $file->filename = $row->field_upload_description[$delta];
          $file_dirty = TRUE;
        }

        if ($file_dirty) {
          file_save($file);
        }
      }
    }

    //resave the node
    if ($node_dirty) {
      foreach($entity->field_upload as $lang => $fields) {
        $entity->field_upload[$lang] = array_values($entity->field_upload[$lang]); //refresh the deltas
      }
      node_save($entity);
    }
  }

  /**
   * @see Migration::preImport()
   */
  public function preImport(){
    parent::preImport();

    //comment_maintain_node_statistics - prevent comment statistics from updating during migration
    $this->comment_maintain_node_statistics = variable_get('comment_maintain_node_statistics', 'deleteme');
    variable_set('comment_maintain_node_statistics', FALSE);
  }

  /**
   * @see Migration::postImport()
   */
  public function postImport(){
    parent::postImport();

    //check if they exist first?
    $this->_registerCommentsMigration();

    //restore comment_maintain_node_statistics
    $var = $this->comment_maintain_node_statistics;
    if ($var == 'deleteme') {
      variable_del('comment_maintain_node_statistics');
    } else {
      variable_set('comment_maintain_node_statistics', $var);
    }
  }

  //helper functions for common file field
  function _has_file_field(){
    $fields = field_info_instances('node', $this->bundle);
    return (isset($fields['field_upload']));
  }

  function _prepare_file_field(&$row){
    $file_directory_path = parent::d6_variable_get('file_directory_path', 'sites/default/files');

    if ($this->_has_file_field()) {
      $files_query = parent::os_migrate_node_get_files_query($row->vid);
      $fields = array(
        'description' => 'description',
        'display' => 'list',
        'filepath' => 'filepath',
        //'file_destination' => 'file_destination',
        //'filename' => 'filename',
        'filename' => 'filename',
        //'field_upload' => 'field_upload',
      );

      $query = $files_query->execute();
      $files = array();
      while ($file_row = $query->fetchObject()) {
        $filepath = $file_row->filepath;
        $filepath = str_replace($file_directory_path, '', $filepath);
        $file = explode('/', trim($filepath, '/'));

        $file_row->filename = array_pop($file);
        $file_row->filepath = 'public://' . implode('/', $file);
        $file_row->field_upload_replace = $filepath;// . '/' . $file_row->filename;
        //$file_row->file_destination = $file_destination;
        $files[] = $file_row;
      }

      parent::os_migrate_prepare_files($files, $row, 'field_upload', $fields);
      //field upload is set up wrong, but it's too late to fix the general function.
      if (isset($row->field_upload)) {
        foreach (array_keys($row->field_upload) as $delta) {
          $row->field_upload[$delta] = $files[$delta]->field_upload_replace;
          //use description as file name
          if (strlen($row->field_upload_description[$delta])) {
            $row->field_upload_filename[$delta] = $row->field_upload_description[$delta];
          }
        }

        //$row->filepath = current($row->field_upload_filepath);
        $filepath = parent::_get_destination_path('node', $row->nid);
        if($filepath){
          //Should always be true for nodes in a site
          $row->field_upload_filepath = $filepath;
        }elseif (is_array($row->field_upload_filepath) && count($row->field_upload_filepath)){
          $row->field_upload_filepath = current($row->field_upload_filepath); //can't use array for this.
        }
//        $row->field_upload_filepath = ($filepath) ? $filepath : current($row->field_upload_filepath); //can't use array for this.
        //$row->filename = $row->field_upload_filename; //expects string?
      }
    }
  }

  function _map_file_field(){
    if ($this->_has_file_field()) {
      $this->addFieldMapping('field_upload', 'field_upload')->defaultValue(NULL);
      $this->addSimpleMappings(parent::subfields('field_upload', array(
        'language',
        'description',
        'display'
      )));
     // $this->addFieldMapping('field_upload:destination_file', 'field_upload_filename'); //can't use this in an array of files
      $this->addFieldMapping('field_upload:source_dir') //->defaultValue(variable_get('os_migrate_files', FALSE));
        ->defaultValue(variable_get('os_migrate_files') . '/' . parent::d6_variable_get('file_directory_path', 'sites/default/files'));
      $this->addFieldMapping('field_upload:destination_dir', 'field_upload_filepath');
      $this->addFieldMapping('field_upload:og_group_ref', 'group_nid');

      //good defaults
      $this->addUnmigratedDestinations(parent::subfields('field_upload', array('file_class','file_replace','preserve_files')));
    }
  }

  function _source_fields_field_upload(){
    if ($this->_has_file_field()) {
      return array(
        'field_upload' => t('Uploaded file'),
        'field_upload:description' => t('Uploaded file\'s desctription'),
        'field_upload:display' => t('Include uploaded file in listing'),
        'field_upload:language' => t('Uploaded file\'s langauge'),
        'filepath' => t('Full path to file'),
        'filename' => t('Name of file'),
      );
    }
    else {
      return array();
    }
  }

  /**
   * @function _registerCommentsMigration
   *
   * Checks if this content type has any comments.  IF so, registers a migration of them
   */
  function _registerCommentsMigration(){
    $query = parent::d6_db_select('comments', 'c')->fields('c', array(
      'nid'
    ));
    $query->join('node', 'n', 'n.nid = c.nid');
    $query->condition('n.type', $this->source_content_type);
    $result = $query->execute();
    $comments = $result->fetchCol();

    if (count($comments)) {
      Migration::registerMigration('CommentsOSMigration', 'commentsosmigration' . $this->bundle, array(
        'bundle' => $this->bundle
      ));
    }
  }

  /**
   * Map bundle names from new to legacy
   */
  public static function get_source_bundle($bundle){
    $og_bundles = og_get_all_group_bundle();
    if ($bundle == self::_d7_vsite_type()) {
      return self::_d6_vsite_type();
    } elseif (array_key_exists($bundle, self::$source_bundle_map)) {
      return self::$source_bundle_map[$bundle];
    }

    return $bundle;
  }

  function _d6_vsite_type() {
    return parent::d6_variable_get('scholar_content_type', 'vsite');
  }

  function _d7_vsite_type() {
    $override = variable_get('os_migrate_default_group_bundle', variable_get('os_default_group_bundle', 'keep'));
    if ($override == 'keep') {
      $d6_vsite = self::_d6_vsite_type();
      if ($d6_vsite == 'project') {
        return 'project';
      } else {
        return 'personal';
      }
    } elseif ($override == 'default') {
      return variable_get('os_default_group_bundle', 'personal');
    } else {
      return $override;
    }
  }

  /**
   * Update links in node bodies.
   */
  public static function _complete_body_links(&$node, $row) {
    //only update nodes with attachments
    $body = $node->body[$node->language][0]['value'];
    $original_body = $body;
    if (!isset($row->field_upload) || empty($node->field_upload[$node->language]) || (strpos($body, 'href=')===FALSE) && (strpos($body, 'src=') === FALSE)) {
      return FALSE;
    }

    //get domain
    $query = parent::d6_db_select('spaces_overrides', 'so')
      ->condition('id', $row->group_nid)
      ->condition('object_id', 'vsite_domain_name')
      ->condition('value', 'N;', '!=')
      ->condition('value', 's:0:"";', '!=')
      ->fields('so', array('value'))
      ->execute();

    if ($query->rowCount()) {
      $domain = $query->fetchField();
      $domain = 'http://' . unserialize($domain);
    }

    $purl_base_domain = variable_get('os_migrate_use_domain', variable_get('purl_base_domain', FALSE));
    if (!isset($domain) || !$domain) {
      $domain = $purl_base_domain;
    }

    //legacy files dir
    $legacy_files = parent::d6_variable_get('file_directory_path', 'sites/default/files');
    $current_files = variable_get('file_directory_path', 'sites/default/files');

    //filepaths to replace.  both filename and files/purl/filename will be appended to each of these.
    $patterns = array(
      "${domain}/${legacy_files}/",    // http://projects.iq.harvard.edu/sites/projects.iq.harvard.edu/files/huqualhomeimage.jpg
      "/${legacy_files}/",             // /sites/projects.iq.harvard.edu/files/rcss/files/ssc-table-01.png
      "${domain}/",                    // http://projects.iq.harvard.edu/sainit/files/SAI_Internship_booklet_2009.pdf
      '/',                             // /govpostgers/files/georgeyin.pdf
    );

    //also check original domain if applicable
    if ($domain != $purl_base_domain) {
      $patterns[] = "${purl_base_domain}/";
      $patterns[] = "${purl_base_domain}/${legacy_files}/";
    }

    /* not supported.  found some paths that are inherently broken.  leaving them that way.
      /sites/projects.iq.harvard.edu/files/thedatanew_guides/files/thedatanew_guides/files/thedatanew_guides/files/simple_study.xml
    */

    //loop over files
    $candidates = array();
    foreach ($row->field_upload as $delta => $filename) {
      if (isset($node->field_upload[$node->language][$delta]) && ($newfile = $node->field_upload[$node->language][$delta])) {
        $newpath = file_create_url($newfile['uri']);
        //$newpath = str_replace($current_files, 'files', $newpath); //swap in files dir.  shouldn't be necessary when configured right
        $newpath = str_replace($GLOBALS['base_url'], $domain, $newpath);

        //try to match cases of path with and without purl, as provided by file table
        $candidates[$filename] = array();
        foreach ($patterns as $p) {
          $candidates[$filename][] = $p . trim($filename, '/');          //full vsite/files/file.ext
          $candidates[$filename][] = $p . end(explode('/', $filename));  //just file.ext
        }

        //add aliases.  aliases only ever follow a domain, so try it for both domain options but no other patterns
        foreach (self::_filefield_get_alias($row->nid, $filename) as $alias) {
          $candidates[$filename][] =  "${domain}/" . trim($alias, '/');
          if ($domain != $purl_base_domain) {
            $candidates[$filename][] =  "${purl_base_domain}/" . trim($alias, '/');
          }
        }

        $candidates[$filename] = array_unique($candidates[$filename]);

        //replace matches in body, wrapped in single/double quotes so we can match whole string.
        foreach ($candidates[$filename] as $c) {
          $body = str_replace('"'.$c.'"', '"'.$newpath.'"', $body);
          $body = str_replace("'".$c."'", "'".$newpath."'", $body);
        }
      }
    }

    //swap body into node return
    if ($body != $original_body) {
      $node->body[$node->language][0]['value'] = $body;
      Migration::displayMessage(t('Updated URLs in node: @nid', array('@nid' => $node->nid)), MigrationBase::MESSAGE_INFORMATIONAL);
      return TRUE;
    }

    return FALSE;
  }

  /**
   * @function _filefield_get_alias
   *
   * Given an uploaded file's nid and filepath, get its fid and use that to return its filefield_paths from url_alias
   * Caches results since it's in the middle of a loop and will be called a dozen or so times per nid/filepath
   *
   * @param int $nid
   *   node id
   * @param str $filepath
   *   path to file, as per legacy db's files table
   */
  static function _filefield_get_alias($nid, $filepath) {
    $filepath = trim($filepath, '/');

    //cache results.  other patterns will use alias
    $nid_path_aliases = &drupal_static(__FUNCTION__, array());
    $nid_path = $nid . '::' . $filepath;
    if (isset($nid_path_aliases[$nid_path])) {
      return $nid_path_aliases[$nid_path];
    }

    $query = parent::d6_db_select('upload', 'u')
      ->condition('nid', $nid);
    $query->join('files', 'f', 'u.fid = f.fid');
    $query->condition('f.filepath', '%'.$filepath, 'LIKE');
    $query->join('url_alias', 'ua', 'ua.src = CONCAT(:path,f.fid)', array(':path'=>'filefield_paths/alias/'));
    $query->fields('ua', array('dst'));

    $result = $query->execute();
    $return = ($result->rowCount()) ? $result->fetchCol() : array();
    $return = array_diff($return, array($filepath)); //don't include original filepath as a result

    $nid_path_aliases[$nid_path] = $return;
    return $return;
  }
}
