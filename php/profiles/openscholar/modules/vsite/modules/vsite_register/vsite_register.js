/**
  * @file vsite_register.js
  *
  * Provides events to change display of select current user/create new user in vsite registration form.
  *
  * Drupal.behaviors.vsite_register.toggle_user_forms swaps display of the new/existing user form elements.  This is determined
  * by new user's display state.  So when both elements are showing, toggling will hide one and show the other.
  *
  * Attaches a click event to call toggle_user_forms.  Tries to do this only once by inspecting $elem.data.  Ajax calls
  * will trigger attach again, causing multiple instances of the same click event to be registered if this isn't done.
  */

(function($) {
  Drupal.behaviors.vsite_register = {
    toggle_user_forms: function() {
      var new_user = $('#new_user_div');
      var old_user = $('#edit-existing-username').parent();

      if (new_user.css('display') == 'none') {
        new_user.css('display', 'block');
        old_user.css('display', 'none');
      } else {
        new_user.css('display', 'none');
        old_user.css('display', 'block');
      }
    },

    attach: function(ctx, settings) {
      if (typeof Drupal.settings.vsite_register == 'undefined') {
        Drupal.settings.vsite_register = {new_user_clicked: false};
      }

      $('#edit-submit').mouseenter(function() {
        // When the submit button is clicked, make sure the focus is on it and
        // not on the "domain" element the avoid Ajax errors.
        $(this).focus();
      })

      //init display of new user xor existing user
      if ($('#new_user_div').css('display') == $('#edit-existing-username').parent().css('display')) {
        Drupal.behaviors.vsite_register.toggle_user_forms();
      }

      //attach click event to new user link.  only do this once, even if this behavior ahppens again
      var link = $('#new-user-link');
      if (link.length) {
        var link_data = link.data();
        if ((typeof link_data['events'] == 'undefined') || link_data['events'].click.length < 1) {
          link.click( function() {
            $('input[name=create_new_user]').attr('value', 1); //store this so form appears right after refresh
            Drupal.behaviors.vsite_register.toggle_user_forms();
          });
        }
      }

      //http://drupal.org/node/1232416   hide the ajax error that comes up when an ajax call is left dangling
      $.ajaxSetup({
        beforeSend: function(jqXHR, settings) {
          settings.error = function(jqXHR, textStatus, errorThrown) {
            //{console.log('ajax error: ' + textStatus);};
          };
        }
      });

      Drupal.detachBehaviors($(this)); //we don't want to set up the hidden element on every page load

    }
  }
})(jQuery);
