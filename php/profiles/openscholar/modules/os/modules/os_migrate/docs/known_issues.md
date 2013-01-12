OpenScholar Migration Known Issues
==================================

## Files

* Some files were renamed due to collisions.  When OS Migrate updates a node with attached files it tries to replace links to the file in the node body with the new file's path, but this isn't 100% reliable and only applies to nodes referring to their own files - nodes linking to another node's files will not be updated.
* File names and descriptions are sometimes missing.  Instead of "Article" the file link would be "article_file.pdf".  Since files use the media module, you will have to edit the file entity rather than the node text to update this.
* Files no longer have the option to be unlisted.  Those files that were attached to a node but left unlisted were migrated to a vsite but will no longer appear in node edit.  They should all be available in cp/content/files
* The media module handles inline embeds of web content.  It creates a stub of a file entity in D7's files tables, even though no actual file exists.  Iframes, embeds, and objects will be passed through the media module and may be rejected.  

## Images

* See file renaming above.
* Cropping and scaling images uses a lot of memory.  So does migrate.  Images that scaled fine when uploaded may run migrate out of memory.  (See here for details: http://drupal.org/node/1635052).  Only vsite logos and profile images used image cropping, so those are the only node types affected.  OS Migrate will avoid scaling images above 1MB, since that was roughly where we had trouble, but YMMV.  To deal with this we identified larger images in advance and replaced them with smaller versions.  Queries like `SELECT nid,filepath,filesize FROM content_type_person ctp INNER JOIN files f ON f.fid = ctp.field_person_photo_fid ORDER BY filesize` can help you find these images.

## Content

* Several nodes had their types updated.  Links and widgets were updated accordingly, but node bodies and anywhere else a link may be hidden were not.
  * announcement -> news
  * gallery -> media_gallery
  * scholar_software_project -> software_project
  * scholar_software_release -> software_release
  * vsite -> personal (there are personal, project, and department nodes.  vsites can become any of these but personal is the default)
* Read more.  The read more link may appear in a different place for content that does not have a user set teaser break.  
* URL Aliases.  Url aliases should be of the form `vsite/page/alias`.  Some were created without the vsite part.  Those migrate as they are.  Unfortunately, D7 matches URL aliases differently than D6 did.  If site1 links to page/alias and has a site1/page/alias url alias, while site2 incorrectly has a page/alias url, site1's link will go to site2's node.  Editing and saving site2's node should correct the alias.  
* Nodes will only migrate if they have a vsite.  This was done to clean up orphaned pages of deleted vsites.  It works as intended, but may be surprising if you were expecting different behavior.

## Widgets

* Blog comments widget has not been migrated.
* CC license widget was not migrated.
* Post in a widget widgets were converted to text/html widgets.  The node was copied to drupal 7, rendered, and saved to HTML.  OS Migrate accounts for most of the weirdness we saw users put in these nodes, but we couldn't find everything, so there may be unexpected results in these boxes.
* Boxes may be out of order if they have the same weight.  When testing found cases of this, we adjusted the d6 site's layout to change the box weights so they wouldn't be the same any more.

## Misc

* The spaces_overrides table uses the MigrateDestinationTableCopy destination.  One complication of this is that it won't roll back individual rows.  Rolling back part of the table will destroy the rest of it AND will fail to alert the other classes using that table that their rows have been deleted.  This isn't a big deal during the standard migration use case, but is a hassle during development.  The workaround we used was to migrate and rollback the spacesoverrides group and ThemeOS class together instead of individually.
