<?php
class MediaInternetEmbedHandler extends MediaInternetBaseHandler {

  public function validate() {
    return media_embed_check_src($this->embedCode);
  }

  public function getFileObject() {
    $uri = 'embed://'. md5($this->embedCode);
    $file = file_uri_to_object($uri);
    $file->type = 'html';

    $file->field_html_code[LANGUAGE_NONE][0]['value'] = $this->embedCode;

    return $file;
  }
/*
 * @see MediaInternetBaseHandler::claim()
 */
  public function claim($embed_code) {
    // remove any tags, then see if the string is still the same
    $stripped = strip_tags($embed_code);
    return $stripped != $embed_code;
  }

}