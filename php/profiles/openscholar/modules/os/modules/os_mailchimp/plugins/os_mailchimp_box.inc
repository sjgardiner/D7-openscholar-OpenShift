<?php

/**
 * @file
 * The class for "mailing list" type widgets.
 */

class os_boxes_mailchimp extends os_boxes_default {
  
  /**
   * Implementation of boxes_content::options_defaults().
   */
  public function options_defaults() {
    $options = parent::options_defaults();
    
    $options['list'] = '';
    $options['text'] = t('Subscribe to our mailing list');
    $options['display'] = 'link';

    return $options;
  }
  
 /**
   * Implementation of boxes_content::options_form().
   */
 public function options_form(&$form_state){

 	$form = parent::options_form($form_state);
	// add a variable for the description
	$description = '';
	// using spaces_get_space instead of vsite_get_vsite to remove the vsite dependency
	if(($space=spaces_get_space()) != FALSE) {
		//openscholar space integratio
		$api = $space->controllers->variable->get('os_mailchimp_api', 'space');
	    // grab the space url and add it into the description
	    $description = 'Choose a mailing list to subscribe users to.  If none are available, check your MailChimp API key in <a href="/'.$space->group->purl.'/cp/settings" target="_new">site settings</a>.';
	} else {
		$api = variable_get('os_mailchimp_api', '');
	    $description = 'Choose a mailing list to subscribe users to.  If none are available, check your MailChimp API key';
	}
	
	if (!$api) {
		$form['subscribe']['api_warning'] = array(
	    	'#markup' => '<div class="error">You have not set a <strong>MailChimp API key</strong>.  Until you do so, mailing lists will not work.  You can set up an API key in <a href="/'.$space->group->purl.'/cp/settings" target="_new">site settings</a>.</div>',
	    	'#type' => 'markup',
		);
	      
	} else {
	    	$form['subscribe']['list'] = array(
	        '#type' => 'select',
	        '#title' => t('Mailing List'),
	        '#default_value' => $this->options['list'],
	        '#options' => _os_mailchimp_get_lists($api),
	        '#description' => $description,
	      );
	      
	      $defaults = $this->options_defaults();
	      $form['subscribe']['text'] = array(
	        '#type' => 'textfield',
	        '#title' => 'Subscribe Text',
	        '#default_value' => $this->options['text'],
	        '#description' => t('Text for subscribe link.  Default: ' . $defaults['text'] . '.'), 
	      );
	      
	      $form['subscribe']['display'] = array(
	        '#type' => 'radios',
	        '#title' => 'Display',
	        '#options' => array('link'=>'Link', 'form'=>'Form'),
	        '#default_value' => $this->options['display'],
	        '#description' => t('Show a link to subscription popup or entire form by default.'),
	      );
	      
	    }
	    
	    
	    return $form;
  }
  
  /**
   * Implementation of boxes_content::render().
   */
    
  public function render() {
    $block = parent::render();
    global $user;
    
    if ($this->options['display'] == 'link') {
      //link to a popup containing the form
      ctools_include('ajax');
      ctools_include('modal');
      ctools_modal_add_js();
      $block['content'] =  ctools_ajax_text_button($this->options['text'], 'os_mailchimp/js/subscribe/'.$this->delta, 'Subscribe to this mailing list', 'ctools-use-modal'); 
  
    } else {
      	//display the full form 
    	// now requires droupal_render to present the form
    	$block['content'] = drupal_render(drupal_get_form('os_mailchimp_form', array('delta' => $this->delta)));
    }
      
    return $block;
  }
}