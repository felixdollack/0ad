/*
CGUI
by Gustav Larsson
gee@pyro.nu

--Overview--

	This is the top class of the whole GUI, all objects
	and settings are stored within this class.

--More info--

	Check GUI.h

*/

#ifndef CGUI_H
#define CGUI_H

#include "ps/Errors.h"
ERROR_GROUP(GUI);
ERROR_TYPE(GUI, JSOpenFailed);

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

#include "GUITooltip.h"
#include "Singleton.h"
#include "lib/input.h"

#include "XML/Xeromyces.h"

extern InReaction gui_handler(const SDL_Event* ev);

//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

//--------------------------------------------------------
//  Types
//--------------------------------------------------------

//--------------------------------------------------------
//  Error declarations
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

/**
 * @author Gustav Larsson
 *
 * Contains a list of values for new defaults to objects.
 */
struct SGUIStyle
{
	// A list of defualts for 
	std::map<CStr, CStr> m_SettingsDefaults;
};

struct JSObject; // The GUI stores a JSObject*, so needs to know that JSObject exists

/**
 * @author Gustav Larsson
 *
 * The main object that includes the whole GUI. Is singleton
 * and accessed by g_GUI.
 *
 * No interfacial functions throws.
 */
class CGUI : public Singleton<CGUI>
{
	friend class IGUIObject;
	friend class IGUIScrollBarOwner;
	friend class CInternalCGUIAccessorBase;

private:
	// Private typedefs
	typedef IGUIObject *(*ConstructObjectFunction)();

public:
	CGUI();
	~CGUI();

	/**
	 * Initializes the GUI, needs to be called before the GUI is used
	 */
	void Initialize();
	
	/**
	 * @deprecated Will be removed
	 */
	void Process();

	/**
	 * Performs processing that should happen every frame (including the "Tick" event)
	 */
	void TickObjects();

	/**
	 * Sends a specified event to every object
	 */
	void SendEventToAll(CStr EventName);

	/**
	 * Displays the whole GUI
	 */
	void Draw();

	/**
	 * Draw GUI Sprite
	 *
	 * @param Sprite Object referring to the sprite (which also caches
	 *        calculations for faster rendering)
	 * @param CellID Number of the icon cell to use. (Ignored if this sprite doesn't
	 *        have any images with "cell-size")
	 * @param Z Drawing order, depth value
	 * @param Rect Position and Size
	 * @param Clipping The sprite shouldn't be drawn outside this rectangle
	 */
	void DrawSprite(CGUISpriteInstance& Sprite, int CellID, const float &Z, 
					const CRect &Rect, const CRect &Clipping=CRect());

	/**
	 * Draw a SGUIText object
	 *
	 * @param Text Text object.
	 * @param DefaultColor Color used if no tag applied.
	 * @param pos position
	 * @param z z value.
	 */
	void DrawText(SGUIText &Text, const CColor &DefaultColor, 
				  const CPos &pos, const float &z, const CRect &clipping);

	/**
	 * Clean up, call this to clean up all memory allocated
	 * within the GUI.
	 */
	void Destroy();

	/**
	 * The replacement of Process(), handles an SDL_Event
	 *
	 * @param ev SDL Event, like mouse/keyboard input
	 */
	InReaction HandleEvent(const SDL_Event* ev);

	/**
	 * Load a GUI XML file into the GUI.
	 *
	 * <b>VERY IMPORTANT!</b> All \<styles\>-files must be read before
	 * everything else!
	 *
	 * @param Filename Name of file
	 */
	void LoadXMLFile(const std::string &Filename);

	/**
	 * Checks if object exists and return true or false accordingly
	 *
	 * @param Name String name of object
	 * @return true if object exists
	 */
	bool ObjectExists(const CStr& Name) const;


	/**
	 * Returns the GUI object with the desired name, or NULL
	 * if no match is found,
	 *
	 * @param Name String name of object
	 * @return Matching object, or NULL
	 */
	IGUIObject* FindObjectByName(const CStr& Name) const;

	/**
	 * The GUI needs to have all object types inputted and
	 * their constructors. Also it needs to associate a type
	 * by a string name of the type.
	 * 
	 * To add a type:
	 * @code
	 * AddObjectType("button", &CButton::ConstructObject);
	 * @endcode
	 *
	 * @param str Reference name of object type
	 * @param pFunc Pointer of function ConstuctObject() in the object
	 *
	 * @see CGUI#ConstructObject()
	 */
	void AddObjectType(const CStr& str, ConstructObjectFunction pFunc) { m_ObjectTypes[str] = pFunc; }

	/**
	 * Update Resolution, should be called every time the resolution
	 * of the OpenGL screen has been changed, this is because it needs
	 * to re-cache all its actual sizes
	 *
	 * Needs no input since screen resolution is global.
	 *
	 * @see IGUIObject#UpdateCachedSize()
	 */
	void UpdateResolution();

	/**
	 * Generate a SGUIText object from the inputted string.
	 * The function will break down the string and its
	 * tags to calculate exactly which rendering queries
	 * will be sent to the Renderer.
	 *
	 * Done through the CGUI since it can communicate with 
	 *
	 * @param Text Text to generate SGUIText object from
	 * @param Font Default font, notice both Default color and default font
	 *		  can be changed by tags.
	 * @param Width Width, 0 if no word-wrapping.
	 * @param BufferZone space between text and edge, and space between text and images.
	 *
	 * pObject is *only* if error parsing fails, and we need to be able to output
	 * which object the error occured in to aid the user. The parameter is completely
	 * optional.
	 */
	SGUIText GenerateText(const CGUIString &Text, const CStr& Font, 
						  const float &Width, const float &BufferZone,
						  const IGUIObject *pObject=NULL);

	/**
	 * Returns the JSObject* associated with the GUI
	 *
	 * @return The relevant JS object
	 */
	JSObject* GetScriptObject() { return m_ScriptObject; }

	/**
	 * Check if an icon exists
	 */
	bool IconExists(const CStr &str) const { return (m_Icons.count(str) != 0); }

	/**
	 * Get Icon (a copy, can never be changed)
	 */
	SGUIIcon GetIcon(const CStr &str) const { return m_Icons.find(str)->second; }

	/**
	 * Get pre-defined color (if it exists)
	 * Returns false if it fails.
	 */
	bool GetPreDefinedColor(const CStr &name, CColor &Output);

private:

	void ClearMouseState();

	/**
	 * Updates the object pointers, needs to be called each
	 * time an object has been added or removed.
	 *
	 * This function is atomic, meaning if it throws anything, it will
	 * have seen it through that nothing was ultimately changed.
	 *
	 * @throws PS_RESULT that is thrown from IGUIObject::AddToPointersMap().
	 */
	void UpdateObjects();

	/**
	 * Adds an object to the GUI's object database
	 * Private, since you can only add objects through 
	 * XML files. Why? Because it enables the GUI to
	 * be much more encapsulated and safe.
	 *
	 * @throws	Rethrows PS_RESULT from IGUIObject::SetGUI() and
	 *			IGUIObject::AddChild().
	 */
	void AddObject(IGUIObject* pObject);

	/**
	 * Report XML Reading Error, should be called from within the
	 * Xerces_* functions.
	 *
	 * @param str Error message
	 */
	void ReportParseError(const char *str, ...);

	/**
	 * You input the name of the object type, and let's
	 * say you input "button", then it will construct a
	 * CGUIObjet* as a CButton.
	 *
	 * @param str Name of object type
	 * @return Newly constructed IGUIObject (but constructed as a subclass)
	 */
	IGUIObject *ConstructObject(const CStr& str);

	/**
	 * Get Focused Object.
	 */
	IGUIObject *GetFocusedObject() { return m_FocusedObject; }

	//--------------------------------------------------------
	/** @name XML Reading Xeromyces specific subroutines
	 *
	 * These does not throw!
	 * Because when reading in XML files, it won't be fatal
	 * if an error occurs, perhaps one particular object
	 * fails, but it'll still continue reading in the next.
	 * All Error are reported with ReportParseError
	 */
	//--------------------------------------------------------

	/**
		Xeromyces_* functions tree
		<objects> (ReadRootObjects)
		 |
		 +-<script> (ReadScript)
		 |
		 +-<object> (ReadObject)
			|
			+-<action>
			|
			+-Optional Type Extensions (IGUIObject::ReadExtendedElement) TODO
			|
			+-�object� *recursive*


		<styles> (ReadRootStyles)
		 |
		 +-<style> (ReadStyle)


		<sprites> (ReadRootSprites)
		 |
		 +-<sprite> (ReadSprite)
			|
			+-<image> (ReadImage)


		<setup> (ReadRootSetup)
		 |
		 +-<tooltip> (ReadToolTip)
		 |
		 +-<scrollbar> (ReadScrollBar)
		 |
		 +-<icon> (ReadIcon)
		 |
		 +-<color> (ReadColor)
	*/
	//@{

	// Read Roots

	/**
	 * Reads in the root element \<objects\> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the objects-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadRootObjects(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the root element \<sprites\> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the sprites-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadRootSprites(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the root element <styles> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the styles-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadRootStyles(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the root element \<setup\> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the setup-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadRootSetup(XMBElement Element, CXeromyces* pFile);

	// Read Subs

	/**
	 * Notice! Recursive function!
	 *
	 * Read in an <object> (the XMBElement) and stores it
	 * as a child in the pParent.
	 *
	 * It will also check the object's children and call this function
	 * on them too. Also it will call all other functions that reads
	 * in other stuff that can be found within an object. Check the
	 * tree in the beginning of this class' Xeromyces_* section.
	 *
	 * Reads in the root element <sprites> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the object-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param pParent	Parent to add this object as child in.
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadObject(XMBElement Element, CXeromyces* pFile, IGUIObject *pParent);

	/**
	 * Reads in the element <script> (the XMBElement) and executes
	 * the script's code.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the sprite-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadScript(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the element <sprite> (the XMBElement) and stores the
	 * result in a new CGUISprite.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the sprite-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadSprite(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the element <image> (the XMBElement) and stores the
	 * result within the CGUISprite.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the image-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param parent	Parent sprite.
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadImage(XMBElement Element, CXeromyces* pFile, CGUISprite &parent);

	// TODO: Documentation. (I'm feeling lazy at the moment.)
	void Xeromyces_ReadEffects(XMBElement Element, CXeromyces* pFile, SGUIImageEffects &effects);

	/**
	 * Reads in the element <style> (the XMBElement) and stores the
	 * result in m_Styles.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the style-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadStyle(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the element <scrollbar> (the XMBElement) and stores the
	 * result in m_ScrollBarStyles.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadScrollBarStyle(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the element <icon> (the XMBElement) and stores the
	 * result in m_Icons.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadIcon(XMBElement Element, CXeromyces* pFile);
	
	/**
	 * Reads in the element <tooltip> (the XMBElement) and stores the
	 * result as an object with the name __tooltip_#.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadTooltip(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the element <color> (the XMBElement) and stores the
	 * result in m_PreDefinedColors
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXMLFile()
	 */
	void Xeromyces_ReadColor(XMBElement Element, CXeromyces* pFile);

	//@}

private:

	// Variables

	//--------------------------------------------------------
	/** @name Miscellaneous */
	//--------------------------------------------------------
	//@{

	/**
	 * An JSObject* under which all GUI JavaScript things will
	 * be created, so that they can be garbage-collected
	 * when the GUI shuts down.
	 */
	JSObject* m_ScriptObject;

	/**
	 * don't want to pass this around with the 
	 * ChooseMouseOverAndClosest broadcast -
	 * we'd need to pack this and pNearest in a struct
	 */
	CPos m_MousePos;

	/**
	 * Indicates which buttons are pressed (bit 0 = LMB,
	 * bit 1 = RMB, bit 2 = MMB)
	 */
	unsigned int m_MouseButtons;

	/// Used when reading in XML files
	// TODO Gee: Used?
	int16_t m_Errors;

	// Tooltip
	GUITooltip m_Tooltip;

	/**
	 * This is a bank of custom colors, it is simply a look up table that
	 * will return a color object when someone inputs the name of that
	 * color. Of course the colors have to be declared in XML, there are
	 * no hard-coded values.
	 */
	std::map<CStr, CColor>	m_PreDefinedColors;

	//@}
	//--------------------------------------------------------
	/** @name Objects */
	//--------------------------------------------------------
	//@{

	/**
	 * Base Object, all its children are considered parentless
	 * because this is not a real object per se.
	 */
	IGUIObject* m_BaseObject;

	/**
	 * Focused object!
	 * Say an input box that is selected. That one is focused.
	 * There can only be one focused object.
	 */
	IGUIObject* m_FocusedObject;

	/** 
	 * Just pointers for fast name access, each object
	 * is really constructed within its parent for easy
	 * recursive management.
	 * Notice m_BaseObject won't belong here since it's
	 * not considered a real object.
	 */
	map_pObjects m_pAllObjects;

	/**
	 * Number of object that has been given name automatically.
	 * the name given will be '__internal(#)', the number (#)
	 * being this variable. When an object's name has been set
	 * as followed, the value will increment.
	 */
	int m_InternalNameNumber;

	/**
	 * Function pointers to functions that constructs
	 * IGUIObjects by name... For instance m_ObjectTypes["button"]
	 * is filled with a function that will "return new CButton();"
	 */
	std::map<CStr, ConstructObjectFunction>	m_ObjectTypes;

	//--------------------------------------------------------
	//	Databases
	//--------------------------------------------------------

	// Sprites
	std::map<CStr, CGUISprite> m_Sprites;

	// Styles
	std::map<CStr, SGUIStyle> m_Styles;

	// Scroll-bar styles
	std::map<CStr, SGUIScrollBarStyle> m_ScrollBarStyles;

	// Icons
	std::map<CStr, SGUIIcon> m_Icons;
};

#endif
