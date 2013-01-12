<?php
class SpacesOverridesBoxesMediaMigration extends OSMigration {
  private $migrated_files = array(); //during rollback, stash the files this box has created here.

  public function __construct() {
    parent::__construct(MigrateGroup::getInstance('spacesoverrides'));

    // spaces overrides has no primary key, so use these
    $key_schema = array(
      'id' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => TRUE
      ),
      'object_type' => array(
        'type' => 'varchar',
        'length' => 64,
        'not null' => TRUE
      ),
      'object_id' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => TRUE
      )
    );

    $fields = array(
      'value' => t('URL'),
      'timestamp' => t('Timestamp'),
      'id' => t('Vsite id')
    );

    // dpm(field_info_bundles('file'));

    $this->destination = new MigrateDestinationMedia('html', 'MigrateFileFid');
    $this->description = t('oEmbed URL -> media migration class.');
    $this->source = new MigrateSourceSQL($this->query(), $fields, null, array(
      'map_joinable' => FALSE
    ));
    $this->map = new MigrateSQLMap($this->machineName, $key_schema, MigrateDestinationMedia::getKeySchema());

    $this->addFieldMapping('og_group_ref', 'id');
    foreach ( array('field_html_code','media_title','media_description') as $field ) {
      $this->addUnmigratedDestinations(array($field,"$field:format","$field:language"), t('DNM')); // these are forged in prepareRow
    }

    $this->addUnmigratedDestinations(array('path'), t('DNM'));
    $this->addSimpleMappings(array('value','timestamp','uid'));
  }
  public function query() {
    $query = parent::d6_db_select('spaces_overrides', 'so')->condition('object_type', 'boxes')->condition('value', '%os_boxes_oembed%', 'LIKE')->fields('so');
    $query->join('node', 'n', 'n.nid = so.id');
    $query->fields('n', array('uid'));
    return $query;
  }
  public function prepareRow(&$row) {
    // value, timestamp
    if (($object = @unserialize($row->value)) === FALSE) {
      parent::saveMessage(t('Could not unserialize embed box'), MigrationBase::MESSAGE_WARNING);
      return FALSE;
    }

    $row->timestamp = $object->changed;
    $selection = $object->options['selection'];
    $html = trim($object->options[$selection]);
    if (empty($html)) {
      parent::saveMessage(t('Embed box had empty html.  Dropping it.  vsite: @id', array('@id'=>$row->id)), MigrationBase::MESSAGE_NOTICE);
      return FALSE;
    }
    // $row->value = 'html_file';

    if ($selection != 'pasted') {

      // html is just a url. get oembed code.
      $transaction = db_transaction(); //$handler->save can fail if the url is not embed friendly.
      try {
        $handler = new MediaInternetOEmbedHandler($html);
        if ($handler->claim($html)) {
          $file = $handler->save();
          $file->uid = $row->uid;
          $file = file_save($file);
        } else {
          parent::saveMessage(t('Could not create OEmbedHandler for url: "@url"', array('@url'=>$html)), MigrationBase::MESSAGE_NOTICE);
          return FALSE;
        }
      } catch (Exception $e) {
        $transaction->rollback();
        parent::saveMessage(t('Could not create OEmbedHandler for url: "@url"', array('@url'=>$html)), MigrationBase::MESSAGE_NOTICE);
        return FALSE;
      }
    } else {
      // html is probably an iframe or something. use it as is.
      // forge an object
      $embed = 'embed://' . md5($html);
      $file = file_uri_to_object($embed);
      $file->uid = $row->uid;
      $file->type = 'html'; // this isn't taking...
      $file->filesize = 0;
      $file->filemime = 'html/embed';
      $file->uri = $embed;
      $file = file_save($file);

      // attach metadata and save
      $obj = entity_metadata_wrapper('file', $file);
      $obj->field_html_code = $object->options[$selection];
      //$obj->field_html_title = $object->title;
      $obj->save();
    }

    if (!$file) {
      parent::saveMessage(t('No file to save: "@id"', array('@id'=>$row->id)), MigrationBase::MESSAGE_WARNING);
      return FALSE;
    }

    //sometimes saving the oembed can fail, but still create a file with no exception.  if so, rollback.
    if (isset($transaction) && !$file->type) {
      $transaction->rollback();
      parent::saveMessage(t('Could not create OEmbedHandler for url: "@url"', array('@url'=>$html)), MigrationBase::MESSAGE_NOTICE);
      return FALSE;
    }

    // now we have a fid to return
    $row->value = $row->fid = $file->fid;

    return parent::prepareRow($row);
  }
  public function preRollback() {
    $this->migrated_files = $this->_migrated_fids();
  }

  public function postRollback() {
    $remaining_files = $this->_migrated_fids();
    $delete_fids = array_filter(array_diff($this->migrated_files, $remaining_files));

    $files = file_load_multiple($delete_fids);
    foreach ($files as $file) {
      file_delete($file);
    }
    //file_delete_multiple($delete_fids);
  }

  function _migrated_fids() {
    return db_select('migrate_map_spacesoverridesboxesmedia', 'mm')
      ->fields('mm', array('destid1'))
      ->execute()
      ->fetchCol();
  }
}