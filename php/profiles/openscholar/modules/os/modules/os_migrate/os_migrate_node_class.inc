<?php

class ClassNodeOSMigration extends AbstractNodeOSMigration {
  public function __construct() {

    // Let AbstractNode handle the source, dest, and map.
    parent::__construct('class'); //current types are vsite, article, and page.  lets start with one of those instead?
    $this->description = t('OS Class Nodes');

    //semester
    $this->addFieldMapping('field_semester', 'field_class_semester_value');

    //year
    $this->addFieldMapping('field_offered_year',  'field_class_year_value');
    $this->addFieldMapping('field_offered_year:from',  'field_class_year_value');
    $this->addFieldMapping('field_offered_year:timezone',  'field_class_year_timezone');
    $this->addUnmigratedDestinations(parent::subfields('field_offered_year', array('rrule', 'to')));

    //link
    $this->addFieldMapping('field_class_link', 'field_class_link_url');
    $this->addFieldMapping('field_class_link:title', 'field_class_link_title');
    $this->addFieldMapping('field_class_link:attributes', 'field_class_link_attributes');


    /************************************************************************
     *  Field mappings tell Migrate which d7 column to deposit d6 data into *
     ************************************************************************/

  }

  public function prepareRow(&$current_row) {

    if(!isset($current_row->field_class_year_timezone)){
      $current_row->field_class_year_timezone = variable_get('date_default_timezone', date_default_timezone_get());
    }

    if(isset($current_row->field_class_year_value)){
      //Expects UTC will be converted to Local Time Zone in date.migrate.inc
      $current_row->field_class_year_value = mktime(0,0,0,1,1,substr($current_row->field_class_year_value, 0,4)); // MigrationBase::timestamp($current_row->field_class_year_value);
    }

    return parent::prepareRow($current_row); //false -> skip this row

  }
}

class ClassMaterialNodeOSMigration extends AbstractNodeOSMigration {
  public function __construct() {

    // Let AbstractNode handle the source, dest, and map.
    parent::__construct('class_material'); //current types are vsite, article, and page.  lets start with one of those instead?
    $this->description = t('OS Class Material Nodes');

    /************************************************************************
     *  Field mappings tell Migrate which d7 column to deposit d6 data into *
    ************************************************************************/

    $this->addFieldMapping('field_class', 'field_class_nid');
  }

  public function prepareRow(&$current_row) {
    return parent::prepareRow($current_row); //false -> skip this row

  }
}