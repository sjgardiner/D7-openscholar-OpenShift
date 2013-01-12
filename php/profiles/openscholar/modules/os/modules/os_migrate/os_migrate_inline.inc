<?php


//copy inline files into file entities

class FileInlineOSMigration extends OSMigration {
  public function __construct() {
    parent::__construct(MigrateGroup::getInstance('inline', array('specialtables')));
    $add_fields = array('uri' => 'Unique Resource Identifier.');
    $this->source = new MigrateSourceSQL($this->query(), $add_fields , NULL,  array('map_joinable' => FALSE));
    $this->map = new MigrateSQLMap($this->machineName,
      array('fid' => array('type'=>'int', 'unsigned'=>TRUE, 'not null'=>TRUE),
             'vid' => array('type'=>'int', 'unsigned'=>TRUE, 'not null'=>TRUE),
        ),
      MigrateDestinationFile::getKeySchema());

    //////$this->destination = new MigrateDestinationFile(array('copy_file'=>TRUE));

    $this->destination = new MigrateDestinationFile('file', 'MigrateFileURI');
    $this->addFieldMapping('value', 'uri');
    $this->addFieldMapping('destination_file', 'filename');
    $this->addFieldMapping('uid', 'uid');

    $this->addFieldMapping('timestamp', 'timestamp');
    //$this->addFieldMapping('path');

    $this->addFieldMapping('destination_dir', 'destination_dir');
    $this->addFieldMapping('source_dir')->defaultValue(
      variable_get('os_migrate_files') . '/' . parent::d6_variable_get('file_directory_path', 'sites/default/files'));


    $this->addUnmigratedDestinations(array('file_replace', 'preserve_files', 'path'), t('DNM'));
    $this->addUnmigratedSources(array('nid', 'field_os_inline_files_fid', 'filepath'), t('DNM'));

  }


  public function query() {
    $query = parent::d6_db_select('content_field_os_inline_files', 'cf')
      ->fields('cf', array('field_os_inline_files_fid', 'nid', 'vid', 'field_os_inline_files_data'))
      ->isNotNull('field_os_inline_files_fid');
    $query->join('files', 'f', 'f.fid = cf.field_os_inline_files_fid');
    $query->fields('f', array('uid', 'filename', 'timestamp',  'filepath', 'fid'));

    return $query;
  }

  public function prepareRow($row) {
    //file::import expects to recieve the file from a url or db blob.  local copy is not available.
    //going with that for now.  may patch migrate if it's obnoxious.
    //this appraoch does not take into account vsite_domain_names from spaces_overrides
    //$purl = parent::d6_variable_get('purl_base_domain');
    $dir = variable_get('os_migrate_files');
    $legacy_uri = $dir . '/' . $row->filepath;

    $file_directory_path = parent::d6_variable_get('file_directory_path', 'sites/default/files');
    $filepath = $row->filepath;
    $filepath = str_replace($file_directory_path, '', $filepath);
    $file = explode('/', trim($filepath, '/'));
    $row->filename = array_pop($file);
    $dest_dir = OSMigration::_get_destination_path('inline_file', $row->fid);
    $row->destination_dir = ($dest_dir) ? $dest_dir :  'public://' . implode('/', $file);
    $row->uri = $filepath;



    if (!parent::legacy_file_exists($legacy_uri)) {
      return FALSE;
    }

  }

  public function prepare(&$file, $row) {
    if ($row->field_os_inline_files_data && ($data = unserialize($row->field_os_inline_files_data))) {
      if (isset($data['description']) && strlen($data['description'])) {
        $file->filename = $data['description'];
      }
    }
  }

  //called immediately after entity is saved
  public function complete($entity, stdClass $row) {

    //update file usage here
    parent::_os_migrate_inline_files_update_file_usage($entity, $row);

  }
}

//for migrating media, MigrateDesinationMedia extends File...




class ImageInlineOSMigration extends OSMigration {
  public function __construct() {
    parent::__construct(MigrateGroup::getInstance('inline', array('specialtables')));
    $this->description = t('Inline Images');
    $add_fields = array('uri' => 'Unique Resource Identifier.');
    $this->source = new MigrateSourceSQL($this->query(), $add_fields , NULL,  array('map_joinable' => FALSE));
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'fid' => array('type'=>'int', 'unsigned'=>TRUE, 'not null'=>TRUE,),
        'vid' => array('type'=>'int', 'unsigned'=>TRUE, 'not null'=>TRUE),
        ),

      //array('os_inline_image_migrate_id' => array('type'=>'varchar'), 'length'=>255, 'not null'=>TRUE, 'description' => 'Unique ids for migrate'),
      MigrateDestinationFile::getKeySchema());
    //$this->destination = new MigrateDestinationFile(array('copy_file'=>TRUE));

    $this->destination = new MigrateDestinationFile('file', 'MigrateFileURI');
    $this->addFieldMapping('value', 'uri');
    $this->addFieldMapping('destination_file', 'filename');

    $this->addFieldMapping('destination_dir', 'destination_dir');
    $this->addFieldMapping('source_dir')->defaultValue(
     variable_get('os_migrate_files') . '/' . parent::d6_variable_get('file_directory_path', 'sites/default/files'));



    // Field mappings //
    $this->addSimpleMappings(array('uid', 'timestamp'));

    //ignored
    $this->addUnmigratedDestinations(array('file_replace', 'preserve_files', 'path'), t('DNM'));
    $this->addUnmigratedSources(array('filepath', 'fid', 'nid', 'field_os_inline_image_fid'), t('DNM'));
  }

  public function query() {

    $query = parent::d6_db_select('content_field_os_inline_image', 'cf')
      ->fields('cf', array('field_os_inline_image_fid', 'nid', 'vid', 'field_os_inline_image_data'  ))
      ->isNotNull('field_os_inline_image_fid')
      ->distinct();
    $query->join('files', 'f', 'f.fid = cf.field_os_inline_image_fid');
    $query->fields('f', array('uid', 'filename', 'timestamp',  'filepath', 'fid'));

    return $query;
  }

  public function prepareRow($row) {
    //file::import expects to recieve the file from a url or db blob.  local copy is not available.
    //going with that for now.  may patch migrate if it's obnoxious.
    //this appraoch does not take into account vsite_domain_names from spaces_overrides
    $dir = variable_get('os_migrate_files');
    $legacy_uri = $dir . '/' . $row->filepath;

    $file_directory_path = parent::d6_variable_get('file_directory_path', 'sites/default/files');
    $filepath = $row->filepath;
    $filepath = str_replace($file_directory_path, '', $filepath);
    $file = explode('/', trim($filepath, '/'));
    $row->filename = array_pop($file);
    $dest_dir = parent::_get_destination_path('inline_image', $row->fid);
    $row->destination_dir = ($dest_dir) ? $dest_dir : 'public://' . implode('/', $file);
    $row->uri = $filepath;

    if (!parent::legacy_file_exists($legacy_uri)) {
      parent::saveMessage(t('Image Migration Error.  File not found: %uri', array('%uri'=>$row->uri)), MigrationBase::MESSAGE_WARNING);
      return FALSE;
    }
  }

  public function prepare(&$file, $row) {
    if ($row->field_os_inline_image_data && ($data = unserialize($row->field_os_inline_image_data))) {
      if (isset($data['description']) && strlen($data['description'])) {
        $file->filename = $data['description'];
      }
    }
  }
}


class EmbedInlineOSMigration extends OSMigration {
  public function __construct() {
    parent::__construct(MigrateGroup::getInstance('inline', array('specialtables')));
    $this->description = t('Inline Embeds');
   // $this->dependencies = array('ImageInlineOSMigration', 'FileInlineOSMigration');

    $add_fields = array('uri' => 'Unique Resource Identifier.', 'filename' => 'File name');
    $this->source = new MigrateSourceSQL($this->query(), $add_fields , NULL,  array('map_joinable' => FALSE));
    $this->map = new MigrateSQLMap($this->machineName,
      array('vid' => array('type'=>'int', 'unsigned'=>TRUE, 'not null'=>TRUE,),
             'delta' => array('type'=>'int', 'unsigned'=>TRUE, 'not null'=>TRUE,),
        ),
      MigrateDestinationMedia::getKeySchema());
      //file dest is no good heree.  oembed:// is unsupported

    $this->destination = new MigrateDestinationFile('file', 'MigrateFileURI');
    $this->addFieldMapping('value', 'uri');
    $this->addFieldMapping('destination_file', 'filename');

    //mappings
    $this->addSimpleMappings(array('uid'));
    $this->addFieldMapping('timestamp', 'created');

    //ignored
    $this->addUnmigratedSources(array('field_os_inline_oembed_url', 'nid'), 'DNM');
    $this->addUnmigratedDestinations(array('destination_dir', 'path', 'file_replace', 'preserve_files', 'source_dir'), 'DNM'); //acceptable defaults

  }

  public function query() {
    $query = parent::d6_db_select('content_field_os_inline_oembed', 'cf')
      ->fields('cf', array('field_os_inline_oembed_url', 'vid', 'delta'))
      ->isNotNull('field_os_inline_oembed_url');
      //->groupBy('field_os_inline_oembed_url');
    $query->distinct();

    $query->leftJoin('node', 'n', 'cf.nid = n.nid');
    $query->fields('n', array('uid', 'created', 'nid'));
    return $query;
  }

  public function prepareRow($row) {
    //$row->uri = $row->field_os_inline_oembed_url;
    //$row->uri = preg_replace('/https?:\/\//', 'oembed://', $row->field_os_inline_oembed_url);//$row->field_os_inline_oembed_url;

    $row->uri = 'oembed://' . $row->field_os_inline_oembed_url;
    $row->filename = md5($row->uri); //or fetch this from media's name getting funcs.  slow as hell?

  }

  //destination forces public:// URI.  switch it back.  uri is also truncated to basename.  lets not make all youtube links the same, kthxbai
  public function prepare($entity, $row) {
    //$entity->uri = $row->uri;//preg_replace('/https?:\/\//', 'oembed://', $row->field_os_inline_oembed_url);
//     $entity->filename = md5($row->uri);
//     $entity->uri = 'embed://' . $entity->filename;
//     $entity->field_html_code = array(LANGUAGE_NONE=>array(array(
//       'value' => '<iframe src="' . $row->field_os_inline_oembed_url . '">',
//     )));

    if (($type = $this->_url_to_type($row->field_os_inline_oembed_url)) == FALSE) {
      $file = new stdClass();
      $file->uri = 'oembed://' . check_url($row->field_os_inline_oembed_url);
      $mime = file_get_mimetype($file->uri);
      $file->filemime = $mime;
      $type = file_get_type($file);
    }
    $entity->type = ($type) ? $type : 'text/oembed';
  }

  //correct the fields that wouldn't attach in prepare
   public function complete($entity, $row) {
     $entity->uri = 'oembed://' . check_plain($row->field_os_inline_oembed_url);
     $entity->filename = check_plain($row->field_os_inline_oembed_url);
     if (strlen($entity->filename) >= 255) {
       $entity->filename = substr($entity->filename, 0, 254);
     }
     file_save($entity);
   }

   function _url_to_type($url) {
     list(,,$domain) = explode('/', $url);
     $urls = array(
       'youtube.com' => 'video',
       'youtu.be' => 'video',
       'soundcloud.com' => 'video',
     );

     return (isset($urls[$domain])) ? $urls[$domain] : FALSE;
   }
}

//import from node bodies too.
//nids are in the sql map.  during node import, check htmlinline's migration table to see if there are
//any previously unmapped files that are now available for use.
class HTMLInlineOSMigration extends OSMigration {
  public function __construct() {
    parent::__construct(MigrateGroup::getInstance('inline', array('specialtables')));
    $this->dependencies = array('SpecialTablesOS');

    $this->description = t('Inline HTML');

    $add_fields = array('uid' => 'User ID', 'timestamp' => 'Node creation time', 'uri' => 'Unique Resource Identifier', 'filename' => 'Filename');
    $this->source = new MigrateSourceSQL($this->query(), $add_fields , NULL,  array('map_joinable' => FALSE));
    $this->map = new MigrateSQLMap($this->machineName,
      array('id' => array('type'=>'int', 'unsigned'=>TRUE, 'not null'=>TRUE,)),
      MigrateDestinationFile::getKeySchema());

    $this->destination = new MigrateDestinationFile('file', 'MigrateFileURI');
    $this->addFieldMapping('value', 'uri');
    $this->addFieldMapping('destination_file', 'filename');

    // Field mappings //
    $this->addSimpleMappings(array('uid', 'timestamp'));

    //ignored
    $this->addUnmigratedDestinations(array('path', 'destination_dir', 'file_replace', 'preserve_files', 'source_dir'), t('DNM')); //only used in copying blobs to files
    $this->addUnmigratedSources(array('vid', 'html'), t('DNM'));
  }

  public function query() {
    $query = db_select('os_migrate_body_inline_objects', 'o')
      ->fields('o', array('id', 'vid', 'html'));
    return $query;
  }

  public function prepareRow($row) {
    $dom = filter_dom_load($row->html);

    //iframe
    if ($iframe = $dom->getElementsByTagName('iframe')->item(0)) {
      $row->uri = $iframe->getAttribute('src');
    } elseif ($embed = $dom->getElementsByTagName('embed')->item(0)) {
      $row->uri = $embed->getAttribute('src');
    } elseif ($object = $dom->getElementsByTagName('object')->item(0)) {
      $row->uri = $object->getAttribute('data'); //flickr
    }


    //object exists, but refers to nothing. drop it.
    if (!strlen($row->uri)) {
      return FALSE;
    }

    //prep the row
    //$row->uri = preg_replace('/https?:\/\//', 'oembed://', $row->uri);

    //get additional data from d6
    $query = parent::d6_db_select('node_revisions', 'nr')
      ->fields('nr', array('uid'))
      ->condition('nr.vid', $row->vid, '=');
    $query->join('node', 'n', 'n.nid = nr.nid');
    $query->fields('n', array('status', 'nid'));
    $node = $query->execute()->fetchAssoc();

    $row->status = $node['status'];
    $row->uid = $node['uid'];
    $row->nid = $node['nid'];

    //let media/oembed fetch file data
    $f = file_uri_to_object($row->uri);
    $row->filename = $f->filename;
    $row->filemime = $f->filemime;

    //$row->path;
  }

  public function prepare(&$entity, $row) {
//    $entity->uri = $row->uri;
    //$entity->value = 'embed://' . md5($row->uri);
    //$entity->value = $entity->uri;

    //treat iframe as embedded html code
    if (strpos($row->html, 'iframe') || strpos($row->html, 'embed')) {
      $entity->type = 'html';
      //uri embed://
      $entity->uri = 'embed://' . md5($row->uri);
      $entity->filemime = 'html/embed';
      $entity->field_html_title = array(LANGUAGE_NONE=>array(array('value'=>'')));
      $entity->field_html_code = array(LANGUAGE_NONE=>array(array(
        'value'=>$row->html,
        'format' => 'filtered_html',
        )));
    }
  }

  public function complete(&$entity, $row) {
    //change uri for iframes
    if (strpos($row->html, 'iframe') || strpos($row->html, 'embed')) {
      $entity->uri = 'embed://' . md5($row->uri);
      file_save($entity);
    }
  }

}