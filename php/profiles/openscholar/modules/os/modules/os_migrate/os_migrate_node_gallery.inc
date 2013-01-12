<?php

class GalleryNodeOSMigration extends AbstractNodeOSMigration {
  public function __construct() {

    // Let AbstractNode handle the source, dest, and map.
    parent::__construct('media_gallery');
    //in d7, images are fields of the media gallery.  in 6, images were a separate content type.  errr.

    //a separate import for projects?
    $this->description = t('OS galleries');

    //unmap fields provided by node abstract that aren't used by media_gallery
    foreach(array('format', 'body:format', 'body:language', 'body:summary', 'teaser') as $field) {
      $this->removeFieldMapping($field, $field);
      $this->addUnmigratedSources(array($field), t('DNM'));
    }
    $this->removeFieldMapping('body', 'body'); //unmap body -> body, but don't DNM it.  media_gallery_descripotion uses it.


    //media_gallery has some features os_image_gallery lacked.  lets set some defaults here since there's nothing to import.
    $defaults = array(
      'media_gallery_columns' => 4,
      'media_gallery_rows' => 4,
      'media_gallery_allow_download' => TRUE,
      'media_gallery_expose_block' => 0, //boxes will handle this for us
      'media_gallery_block_columns' => 1,
      'media_gallery_block_rows' => 1,
    );
    foreach ($defaults as $field => $value) {
      $this->addFieldMapping($field)->defaultValue($value);
    }

    /* Field: description */
    $this->addFieldMapping('media_gallery_description', 'body');
    $this->addFieldMapping('media_gallery_description:format')
        ->defaultValue('filtered_html');
    $this->addFieldMapping('media_gallery_description:language', 'language');

    //field: media_gallery_media

    $this->addFieldMapping('media_gallery_media', 'media_gallery_media');
    $this->addFieldMapping('media_gallery_media:destination_dir', 'media_gallery_media:destination_dir');
    //$this->addFieldMapping('media_gallery_media:destination_file', 'media_gallery_media:filename');
    $this->addFieldMapping('media_gallery_media:source_dir')
      ->defaultValue(variable_get('os_migrate_files') . '/' . parent::d6_variable_get('file_directory_path', 'sites/default/files'));

    $this->addFieldMapping('media_gallery_media:description', 'media_gallery_media_description');
    $this->addFieldMapping('media_gallery_media:display', 'media_gallery_media_display')->defaultValue(1);
    $this->addFieldMapping('media_gallery_media:language', 'media_gallery_media_language');

    $this->addFieldMapping('media_gallery_media:og_group_ref', 'group_nid');


    $this->addUnmigratedDestinations(parent::subfields('media_gallery_media', array('file_class', 'file_replace', 'preserve_files')), t('DNM')); //happy with defaults
    $this->addUnmigratedDestinations(array('media_gallery_image_info_where'), t('DNM'));

    $this->addFieldMapping('media_gallery_format')
      ->defaultValue('lightbox'); //use the lightbox by default.  no previous setting to import.
    $this->addFieldMapping('media_gallery_lightbox_extras')
      ->defaultValue(0); //no title/description in lightbox because no images have them.  no previous setting


  }

  public function sourceFields($content_type) {
    $fields = parent::sourceFields($content_type);

    $fields +=  array(
    	'media_gallery_media' => t('Images in gallery.'),
    	'media_gallery_media_description' => t('Image\'s text description'),
   // 	'media_gallery_media_display' => t('Image\'s listing status'),
      'media_gallery_media_language' => t('Language used in image\'s description'),

      'body:format' => t('Provided by node abstract, but ignored here.'),
    	'body:language' => t('Provided by node abstract, but ignored here.'),
    	'body:summary' => t('Provided by node abstract, but ignored here.'),
    );

    return $fields;
  }

  public function query($content_type) {
    $query = parent::query($content_type);

    //$query = parent::d6_db_select('node', 'n')->fields('n', $this->getNodeFields());
    //$query->condition('n.type', 'gallery', '=');



    //$query->join('purl', 'p', 'n.nid = p.id');
    //$query->fields('p', array('value', 'provider'));
    //node abstract includes revisions table.  not an option for vsites.  define query here to avoid the abstract one.
    return $query;


  }
  function prepareRow($current_row) {

    //In testing there were a handful of instances of images in someone else's gallery.
    //Views' spaces_current_space prevented those from showing up in the gallery.
    //Let's just remove them here instead by checking against the og group id.
    $og_id = parent::d6_db_select('og_ancestry')
      ->fields('og_ancestry', array('group_nid'))
      ->condition('nid', $current_row->nid, '=')
      ->execute()
      ->fetchField();

    //grab associated images.
    $query = parent::d6_db_select('node', 'n');

    $query->join('content_type_image', 'cti', 'n.nid = cti.field_gallery_nid');
    $query->condition('field_gallery_nid', $current_row->nid, '=');
    $query->addField('cti', 'field_gallery_image_list', 'list');
    $query->addField('cti', 'field_gallery_image_data', 'description');

    $query->join('files', 'f', 'f.fid = cti.field_gallery_image_fid');
    $query->fields('f', array('filepath', 'timestamp'));

    //join associated image node as img.  a little confusing since we have two nodes here
    $query->join('node', 'img', 'img.nid = cti.nid');
    $query->fields('img', array('title', 'language')); //no galleries or images actually have a language...

    //gallery will be sorted in order that these images are added.  therefore we order them by d6's view sort parameters
    $query->orderBy('img.sticky', 'DESC');
    $query->orderBy('img.created', 'ASC');
    $query->orderBy('img.title', 'ASC');

    //see $og_id above
    $query->join('og_ancestry', 'img_og', 'img.nid = img_og.nid');
    $query->condition('img_og.group_nid', $og_id, '=');

    $result = $query->execute();
    //$current_row->prepared_files = parent::os_migrate_prepare_files($result);
    //this gets subfields, but media_gallery_media still has nothing
    $fields = array('language' => 'language', 'description' => 'description', 'display' => 'list', 'title' => 'title');
    //parent::os_migrate_prepare_files_v24($result, $current_row, 'media_gallery_media', $fields);
    parent::os_migrate_prepare_files($result, $current_row, 'media_gallery_media', $fields);

    if (isset($current_row->media_gallery_media)) {
      foreach($current_row->media_gallery_media as $delta => $filepath) {
        if (list($path, $name) = $this->_migrate_filename($filepath)) {
          $current_row->{'media_gallery_media:filename'}[$delta] = $name;
          $current_row->{'media_gallery_media'}[$delta] = $path . '/' . $name;
        }
      }
      $current_row->{'media_gallery_media:destination_dir'} = parent::_get_destination_path('node', $current_row->nid);
    } else {
      parent::saveMessage(t('Gallery has no media to migrate: @nid', array('@nid'=>$current_row->nid)), MigrationBase::MESSAGE_WARNING);
    }

    //description may be serialized array
    if (isset($current_row->media_gallery_media_description) && is_string($current_row->media_gallery_media_description)) {
      if ($desc = @unserialize($current_row->media_description)) {
        $current_row->media_gallery_media_description = (isset($desc['description'])) ? $desc['description'] : '';
      }
    }

  /*  $current_row->purl = array(
    	'value' => $current_row->value,
      'provider' => $current_row->provider,
      'id' => $current_row->nid,
    );
    */
    return parent::prepareRow($current_row); //false -> skip this row

  }

  /**
   * Implements complete
   *
   * media_gallery doesn't provide a title field on the media files.  save it by hand after saving the node.
   */
  function complete(&$entity, $row) {
    //attach titles to files.
    if (isset($entity->media_gallery_media) && isset($entity->media_gallery_media[$entity->language])) {
      foreach ($entity->media_gallery_media[$entity->language] as $delta => $file) {
        $image = file_load($file['fid']);
        if ($image->media_title[$entity->language][] = array('value' => $row->media_gallery_media_title[$delta])) {
          file_save($image);
        }
      }
    }
  }

}