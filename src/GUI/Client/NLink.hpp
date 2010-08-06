#ifndef CF_GUI_Client_NLink_hpp
#define CF_GUI_Client_NLink_hpp

//////////////////////////////////////////////////////////////////////////////

#include <QObject>

#include "GUI/Client/CNode.hpp"

class QMenu;

//////////////////////////////////////////////////////////////////////////////

namespace CF {

namespace Common { class CPath; }

namespace GUI {
namespace Client {

  ////////////////////////////////////////////////////////////////////////////

  /// @brief Client corresponding component for @c CF::Common::CLink.get
  class NLink :
      public QObject,
      public CNode
  {
    Q_OBJECT

  public:

    typedef boost::shared_ptr<NLink> Ptr;
    typedef boost::shared_ptr<NLink const> ConstPtr;

    /// @brief Constructor
    /// @param name Link name
    /// @param targetPath Target path
    NLink(const QString & name, const CF::Common::CPath & targetPath);

    /// @brief Gives the icon associated to this node
    /// @return Returns the icon associated to this node
    /// @note This method should be reimplemented by all subclasses.
    virtual QIcon getIcon() const;

    /// @brief Gives the text to put on a tool tip
    /// @return The name of the class.
    virtual QString getToolTip() const;

    /// @brief Gives the target path
    /// @return Returns the target path.
    CF::Common::CPath getTargetPath() const;

  private slots:

    /// @brief Slot called when user wants to switch to the target
    void goToTarget();

  public slots:

    /// @brief Slot called when user wants to change the target path
    void changeTargetPath();

  private :

    /// @brief Target path
    CF::Common::CPath m_targetPath;

    /// @brief Context menu
    static QMenu * m_menu;

    /// regists all the signals declared in this class
    static void regist_signals ( Component* self ) {}

  }; // class NLink

//////////////////////////////////////////////////////////////////////////////

} // namespace Client
} // namespace GUI
} // namespace CF

//////////////////////////////////////////////////////////////////////////////

#endif // CF_GUI_Client_NLink_hpp
