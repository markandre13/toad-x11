/**
 * \class toad::TModel
 *
 * \note
 *   \li
 *     MacApp uses an optional Mark'n Sweep algorithm 
 *     (http://developer.apple.com/techpubs/mac/MacAppProgGuide/MacAppProgGuide-26.html)
 *     to distribute modification information.
 *   \li
 *     Cocoa: AppKit doesn't contain the word Model in our context. 
 *     NSResponder? event message, action message, responder chain?
 *   \li
 *     Swing: ?
 *   \li
 *     we want easy maintainable connections and disconnections between 
 *     'model and view' and 'model and model'.
 *   \li
 *     we need a way to connect entities of different type (ie. numbers
 *     to be edited via TTextArea via TTextModel)
 *   \li
 *     models have to keep track of the undo and redo list
 *   \li
 *     ...
 */
