<?php
class os_boxes_html extends os_boxes_default {

  public function options_defaults() {
    $options = parent::options_defaults();

    $options['text'] = array(
      'value' => '',
      'format' => filter_default_format(),
    );
    $options['format'] = 1;

    return $options;
  }

  public function options_form(&$form_state) {
    $form = parent::options_form($form_state);

    $form['text'] = array(
      '#type' => 'text_format',
      '#default_value' => $this->options['text']['value'],
      '#format' => isset($this->options['text']['format'])?$this->options['text']['format']:NULL,
      '#title' => t('Content'),
      '#description' => t('You can put almost anything into here to place it in a widget. Text, HTML, Images, Video, whatever works in the editor.')
    );
    return $form;
  }

  public function options_submit($form, $form_state) {
    if (module_exists('vsite') && $space = spaces_get_space()) {
      // construct a fake node so we can pass it into os_wysiwyg's helper function
      $space->group->body[LANGUAGE_NONE][0]['value'] = $this->options['text']['value'];

      os_wysiwyg_update_files($space->group);
    }
  }

  public function render() {
    $block = parent::render();

    $block['content'] = check_markup($this->options['text']['value'], $this->options['text']['format']);

    return $block;
  }
}