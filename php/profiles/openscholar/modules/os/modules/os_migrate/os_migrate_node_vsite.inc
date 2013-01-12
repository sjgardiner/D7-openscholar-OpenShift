<?php

class VsiteNodeOSMigration extends AbstractNodeOSMigration {

  //Save the image toolkit that is active when the migration starts
  public static $image_toolkit;

  public function __construct() {
    // Let AbstractNode handle the source, dest, and map.
    parent::__construct($this->_d7_vsite_type()); //current types are vsite, article, and page.  lets start with one of those instead?
    //a separate import for projects?
    $this->description = t('OS vsites');
    $this->dependencies = preg_grep('/^VsiteNodeOS$/', $this->dependencies, PREG_GREP_INVERT);
    $this->dependencies[] = 'PurlVsiteDomainTable';

    //$this->addSimpleMappings(array('purl'));

    $this->addFieldMapping('field_site_description', 'og_description');
    $this->addFieldMapping('field_site_description:language')->defaultValue('und');
    $this->addFieldMapping('field_site_description:format')->defaultValue('plain_text');

    $this->addFieldMapping('field_site_address', 'field_vsite_address_value');
    $this->addFieldMapping('field_site_address:language')->defaultValue('und');
    $this->addFieldMapping('field_site_address:format')->defaultValue('plain_text');

    $this->addFieldMapping('group_access', 'og_private')->defaultValue(0);
    $this->addFieldMapping('group_group')->defaultValue(1);

    $this->addFieldMapping('field_site_logo', 'field_site_logo')->defaultValue(NULL);
    $this->addSimpleMappings(parent::subfields('field_site_logo', array('alt', 'title', 'cropbox_x', 'cropbox_y', 'language')));
    $this->addFieldMapping('field_site_logo:cropbox_width', 'field_site_logo:cropbox_width');//->defaultValue(200);
    $this->addFieldMapping('field_site_logo:cropbox_height', 'field_site_logo:cropbox_height');//->defaultValue(150);

    $this->addUnmigratedDestinations(parent::subfields('field_site_logo', array('file_class', 'file_replace', 'preserve_files')), t('DNM'));
    $this->addFieldMapping('field_site_logo:source_dir')->defaultValue(variable_get('os_migrate_files', ''));
    $this->addFieldMapping('field_site_logo:destination_file', 'field_site_logo:destination_file');
    $this->addFieldMapping('field_site_logo:destination_dir', 'field_site_logo:destination_dir');

    //field_group_path
    $this->addSimpleMappings(array('field_group_path', 'field_group_path:attributes'));
    $this->addFieldMapping('field_group_path:title')->defaultValue('');

    //undo what nodeabstract has done
    $unmap = array_merge(array('body', 'group_nid', 'og_group_ref', 'type'),
        parent::subfields('body', array('format', 'language', 'summary')));
    foreach ($unmap as $destination_field) {
      $this->removeFieldMapping($destination_field);
    }


    $this->addUnmigratedSources(array('value', 'provider', 'format', 'field_vsite_address_format'), t('DNM'));
    $this->addUnmigratedSources(parent::subfields('field_vsite_logo', array('fid','list','data'), '_'), t('DNM'));
  }

  public function query() {
    $vsite = $this->_d6_vsite_type();
    $query = parent::d6_db_select('node', 'n')->fields('n', array('nid', 'vid', 'type','language','title','uid','status','created', 'changed','comment','promote','moderate','sticky','tnid','translate'));
    $query->condition('n.type', $vsite);


    $query->join("content_type_$vsite", 'ct', 'n.nid = ct.nid');
    $query->fields('ct', parent::subfields("field_$vsite", array('logo_fid', 'logo_list', 'logo_data'), '_'));
    if ($vsite == 'vsite') {
      $query->fields('ct', array('field_vsite_address_value'));
    }
    //$query->isNotNull('field_vsite_logo_fid'); //uncomment to test logo presence

    $query->join('purl', 'p', 'n.nid = p.id');
    $query->fields('p', array('value', 'provider'));
    $query->condition('p.provider', 'spaces_og', '='); //domain sites had an extra entry for vsite_domain.  add those elsewehre later

//     $query->join('og_ancestry', 'oga', 'n.nid = oga.nid'); //node membership
//     $query->fields('oga', array('group_nid'));

    $query->join('og', 'og', 'n.nid = og.nid');
    $query->fields('og', parent::subfields('og', array('description', 'language', 'private'), '_'));

    if ($restrict = OSMigration::restrict_vsites()) {
      $query->condition('n.nid', $restrict, 'IN');
    }

    return $query;
  }

  public function sourceFields($content_type) {
    $fields = parent::sourceFields($content_type);
    $fields['nr_uid'] = t('Revision\'s owner');
    $fields['log'] = t('Log');
    $fields['format_name'] = t('Format name');
    $fields['field_site_logo'] = t('Site Logo');

    foreach(parent::subfields('field_site_logo', array('alt','title','cropbox_x','cropbox_y','language','cropbox_width','cropbox_height', 'destination_file')) as $field) {
      $fields[$field] = t('Site logo subfield: @field', array('@field' => $field));
    }

    return $fields;
  }

  function prepareRow($row) {
    $vsite = $this->_d6_vsite_type();

    $field_vsite_logo = "field_${vsite}_logo";
    $field_vsite_logo_fid = "field_${vsite}_logo_fid";
    $field_vsite_logo_data = "field_${vsite}_logo_data";

     if (isset($row->$field_vsite_logo_fid)) {
       $file = parent::get_file_by_fid($row->$field_vsite_logo_fid);
       list($dest_dir, $dest_file) = parent::_migrate_filename($file->filepath);
       //$row->{'field_site_logo:destination_file'} = $file->filepath;
       $row->{'field_site_logo:destination_file'} = $dest_file;
       $filepath = parent::_get_destination_path('og', $row->nid); //this is vsite.  nid is the group nid.
       $row->{'field_site_logo:destination_dir'} = ($filepath) ? $filepath : $dest_dir;

       //if we have crop data and the original image still exists, use that instead of the pre-cropped image.
//        $original_file = $file->filepath . '.crop_display.jpg';
//        if (is_readable(variable_get('os_migrate_files', '') . '/' . $original_file) && strlen($row->$field_vsite_logo_data)) {
//          $file->filepath = $original_file;
//        }

       if (!is_readable(variable_get('os_migrate_files', '') . '/' . $file->filepath)) {
         Migration::saveMessage(t('Could not read file @file.  Ignoring it.', array('@file' => $file->filepath)), Migration::MESSAGE_NOTICE);
       } else {

         //attach logo
         parent::os_migrate_prepare_files(array($file), $row, 'field_site_logo');
         $row->{'field_site_logo:language'} = $row->language;

         if (strlen($row->$field_vsite_logo_data) && ($data = unserialize($row->$field_vsite_logo_data))) {
           $row->{'field_site_logo:alt'} = isset($data['alt']) ? $data['alt'] : '' ;
           $row->{'field_site_logo:title'} = isset($data['title']) ? $data['title'] : '';

           //add image dimensions, or defaults as needed
           if (isset($data['crop']['x'], $data['crop']['y'])) {
             $row->{'field_site_logo:cropbox_x'} = max((int)$data['crop']['x'], 0); //yes someone has a negative origin in prod.
             $row->{'field_site_logo:cropbox_y'} = max((int)$data['crop']['y'], 0);
           }

           if (isset($data['crop']['height'], $data['crop']['width'])) {
             $row->{'field_site_logo:cropbox_height'} = (int)$data['crop']['height'];
             $row->{'field_site_logo:cropbox_width'} = (int)$data['crop']['width'];
           }

           if (!isset($row->{'field_site_logo:cropbox_x'}, $row->{'field_site_logo:cropbox_y'}, $row->{'field_site_logo:cropbox_width'},$row->{'field_site_logo:cropbox_height'}) ) {
             $this->_crop_full_image($row, $file->filepath);
           }
         } else {
           if (!$this->_get_missing_coords($row)) {
             //image provided, but no data.
           }
         }
       }
     } else {
       $row->$field_vsite_logo = null;
     }

     //update featured site flag to sticky
     if ($flag_id = parent::d6_db_select('flags')->condition('name', 'featured_web_sites')->fields('flags', array('fid'))->execute()->fetchField()) {
       $flag_query = parent::d6_db_select('flag_content', 'fc')
         ->condition('fc.fid', $flag_id)
         ->condition('content_id', $row->nid)
         ->fields('fc', array('fid'))
         ->execute();
       $row->sticky = (bool) $flag_query->rowCount();
     }

     $row->field_group_path = $this->_get_field_group_path($row->nid, $row->value);
     $row->{'field_group_path:attributes'} = array();

     return;
   }

   function prepare($entity, $current_row) {
     //purl isn't a valid destination yet, so just attach this to the entity.
     $entity->purl = array(
         'value' => $current_row->value,
         'provider' => $current_row->provider,
         'id' => $current_row->nid,
     );

     //set private site
     if (module_exists('vsite_access')) {
       $privacy = parent::d6_db_select('spaces_overrides', 'so')
         ->condition('id', $entity->nid)
         ->condition('object_id', 'vsite_private')
         ->fields('so', array('value'))
         ->execute()
         ->fetchField();

       if ($privacy && (unserialize($privacy)==1)) {
         $entity->group_access[LANGUAGE_NONE][0]['value'] = 1;
       }
     }
   }



   /**
    * @function _get_field_group_path($nid)
    * @param (int) $nid
    *   Vsite id
    * @return string
    *   path to this group
    *
    * Check domain, group, then local
    */
  function _get_field_group_path($nid, $purl) {
    $vsite_domain = parent::d6_db_select('purl', 'p')
      ->condition('id', $nid)
      ->condition('provider', 'vsite_domain')
      ->fields('p', array('value'))
      ->execute()
      ->fetchColumn();

    if ($vsite_domain) {
      return 'http://' . $vsite_domain;
    }

    $override = parent::d6_db_select('spaces_overrides', 'so')
      ->condition('id', $nid)
      ->condition('so.object_type', 'variable')
      ->condition('so.object_id', 'vsite_domain_name')
      ->fields('so', array('value'))
      ->execute()
      ->fetchColumn();

    $override = @unserialize($override);
    if ($override) {
      return 'http://' . $override . '/' . $purl;
    }

    $domain = '[site:url]';
    return $domain . $purl;
  }

  //some vsites have logos, but no coords.  if they're here and hardcoded, use them.
  function _get_missing_coords(&$row) {
    $known_missing = array(
      //nid => x, y, w, h
      2 => array(0, 0, 1114, 836),
      5 => array(0, 0, 789, 592),
      7 => array(0, 0, 2304, 1728),
      13 => array(0, 0, 350, 263),
      17 => array(0, 0, 180, 140),
      19 => array(0, 0, 251, 188),
      21 => array(0, 0, 180, 140),
      25 => array(0, 0, 581, 436),
      26 => array(0, 0, 312, 243),
      541 => array(0, 0, 99, 77),
      619 => array(0, 0, 595, 463),
      660 => array(0, 0, 140 , 105),
      1487 => array(0, 0, 175, 128),
      1791 => array(0, 0, 180, 140),
      1793 => array(0, 0, 180, 140),
      1813 => array(0, 0, 180, 140),
      1815 => array(0, 0, 180, 140),
      1836 => array(0, 0, 180, 140), //doesn't use the widget.  assuming default.
      1857 => array(0, 0, 180, 140),
    );

    if (isset($known_missing[$row->nid])) {
      $crop = $known_missing[$row->nid];
      $row->{'field_site_logo:cropbox_x'} = $crop[0];
      $row->{'field_site_logo:cropbox_y'} = $crop[1];
      $row->{'field_site_logo:cropbox_width'} = $crop[2];
      $row->{'field_site_logo:cropbox_height'} = $crop[3];
      return TRUE;

    } else {
      return FALSE;
    }

  }

  /**
   * @function _crop_full_image
   *
   * No cropping data exists.  Use the full image.  This is correct behavior for project logos.
   */
  function _crop_full_image(&$row, $filepath) {
    $filepath = variable_get('os_migrate_files', '') . '/' . $filepath;

    if (($dim = getimagesize($filepath)) == FALSE) {
      return FALSE;
    }
    $row->{'field_site_logo:cropbox_x'} = 0;
    $row->{'field_site_logo:cropbox_y'} = 0;
    $row->{'field_site_logo:cropbox_width'} = $dim[0];
    $row->{'field_site_logo:cropbox_height'} = $dim[1];
  }
}