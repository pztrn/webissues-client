/**************************************************************************
* This file is part of the WebIssues Desktop Client program
* Copyright (C) 2006 Michał Męciński
* Copyright (C) 2007-2012 WebIssues Team
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "issuedialogs.h"

#include "application.h"
#include "commands/issuebatch.h"
#include "data/datamanager.h"
#include "data/entities.h"
#include "data/issuetypecache.h"
#include "data/localsettings.h"
#include "dialogs/messagebox.h"
#include "utils/definitioninfo.h"
#include "utils/viewsettingshelper.h"
#include "utils/attributehelper.h"
#include "utils/formatter.h"
#include "utils/iconloader.h"
#include "widgets/inputlineedit.h"
#include "widgets/markuptextedit.h"
#include "widgets/abstractvalueeditor.h"
#include "widgets/valueeditorfactory.h"
#include "widgets/separatorcombobox.h"

#include <QLayout>
#include <QLabel>
#include <QRadioButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QButtonGroup>
#include <QLineEdit>
#include <QFileInfo>
#include <QRegExp>

IssueDialog::IssueDialog( QWidget* parent ) : CommandDialog( parent ),
    m_nameEdit( NULL ),
    m_descriptionEdit( NULL )
{
}

IssueDialog::~IssueDialog()
{
    if ( !isFixed() )
        application->applicationSettings()->setValue( "IssueDialogSize", size() );
}

bool IssueDialog::initialize( int typeId, int projectId, bool withDescription )
{
    IssueTypeCache* cache = dataManager->issueTypeCache( typeId );
    QList<int> attributes = cache->attributes();

    bool membersRequired = false;

    foreach ( int attributeId, attributes ) {
        DefinitionInfo info = cache->attributeDefinition( attributeId );
        if ( AttributeHelper::toAttributeType( info ) == UserAttribute ) {
            if ( info.metadata( "required" ).toBool() && info.metadata( "members" ).toBool() ) {
                membersRequired = true;
                break;
            }
        }
    }

    if ( membersRequired ) {
        ProjectEntity project = ProjectEntity::find( projectId );
        if ( project.members().isEmpty() ) {
            showWarning( tr( "There are no available project members to assign to the issue." ) );
            showCloseButton();
            setContentLayout( NULL, true );
            return false;
        }
    }

    QGridLayout* layout = new QGridLayout();

    QLabel* nameLabel = new QLabel( tr( "&Name:" ), this );
    layout->addWidget( nameLabel, 0, 0 );

    m_nameEdit = new InputLineEdit( this );
    m_nameEdit->setMaxLength( 80 );
    m_nameEdit->setRequired( true );
    layout->addWidget( m_nameEdit, 0, 1 );

    nameLabel->setBuddy( m_nameEdit );

    QTabWidget* tabWidget = NULL;

    if ( !attributes.isEmpty() || withDescription ) {
        tabWidget = new QTabWidget( this );
        layout->addWidget( tabWidget, 1, 0, 1, 2 );
    }

    if ( !attributes.isEmpty() ) {
        QWidget* attributeTab = new QWidget( tabWidget );
        tabWidget->addTab( attributeTab, IconLoader::icon( "attribute" ), tr( "Attributes" ) );

        QVBoxLayout* attributeLayout = new QVBoxLayout( attributeTab );

        QScrollArea* attributeScroll = new QScrollArea( attributeTab );
        attributeScroll->setWidgetResizable( true );
        attributeLayout->addWidget( attributeScroll );

        QPalette scrollPalette = palette();
        scrollPalette.setColor( QPalette::Window, QColor::fromRgb( 255, 255, 255 ) );
        attributeScroll->setPalette( scrollPalette );

        QWidget* attributePanel = new QWidget( attributeScroll );
        attributeScroll->setWidget( attributePanel );

        QGridLayout* valuesLayout = new QGridLayout( attributePanel );

        valuesLayout->setColumnMinimumWidth( 0, 120 );

        QLabel* nameLabel = new QLabel( tr( "Name" ), attributePanel );
        valuesLayout->addWidget( nameLabel, 0, 0 );

        QLabel* valueLabel = new QLabel( tr( "Value" ), attributePanel );
        valuesLayout->addWidget( valueLabel, 0, 1 );

        QFrame* separator = new QFrame( attributePanel );
        separator->setFrameStyle( QFrame::HLine | QFrame::Sunken );
        valuesLayout->addWidget( separator, 1, 0, 1, 2 );

        for ( int i = 0; i < attributes.count(); i++ ) {
            int attributeId = attributes.at( i );

            QLabel* headerLabel = new QLabel( cache->attributeName( attributeId ), attributePanel );
            valuesLayout->addWidget( headerLabel, i + 2, 0 );

            DefinitionInfo info = cache->attributeDefinition( attributeId );

            AbstractValueEditor* editor = ValueEditorFactory::createValueEditor( info, projectId, this, attributePanel );
            valuesLayout->addWidget( editor->widget(), i + 2, 1 );

            m_editors.insert( attributeId, editor );
        }

        valuesLayout->setRowStretch( attributes.count() + 2, 1 );
    }

    if ( withDescription ) {
        QWidget* descriptionTab = new QWidget( tabWidget );
        tabWidget->addTab( descriptionTab, IconLoader::icon( "description-new" ), tr( "Description" ) );

        QVBoxLayout* descriptionLayout = new QVBoxLayout( descriptionTab );

        m_descriptionEdit = new MarkupTextEdit( descriptionTab );
        m_descriptionEdit->setRequired( false );
        descriptionLayout->addWidget( m_descriptionEdit );
    }

    setContentLayout( layout, tabWidget == NULL );

    m_nameEdit->setFocus();

    resize( application->applicationSettings()->value( "IssueDialogSize", QSize( 740, 550 ) ).toSize() );

    return true;
}

void IssueDialog::setIssueName( const QString& name )
{
    m_nameEdit->setInputValue( name );
}

QString IssueDialog::issueName() const
{
    return m_nameEdit->inputValue();
}

void IssueDialog::setAttributeValue( int attributeId, const QString& value )
{
    AbstractValueEditor* editor = m_editors.value( attributeId );
    if ( editor )
        editor->setInputValue( value );
}

QString IssueDialog::attributeValue( int attributeId ) const
{
    AbstractValueEditor* editor = m_editors.value( attributeId );
    if ( editor )
        return editor->inputValue();
    return QString();
}

QList<int> IssueDialog::attributeIds() const
{
    return m_editors.keys();
}

void IssueDialog::setDescriptionText( const QString& text )
{
    m_descriptionEdit->setInputValue( text );
}

QString IssueDialog::descriptionText() const
{
    return m_descriptionEdit->inputValue();
}

void IssueDialog::setDescriptionFormat( TextFormat format )
{
    m_descriptionEdit->setTextFormat( format );
}

TextFormat IssueDialog::descriptionFormat() const
{
    return m_descriptionEdit->textFormat();
}

AddIssueDialog::AddIssueDialog( int folderId, int cloneIssueId, QWidget* parent ) : IssueDialog( parent ),
    m_folderId( folderId )
{
    FolderEntity folder = FolderEntity::find( folderId );
    IssueEntity issue = IssueEntity::find( cloneIssueId );

    if ( cloneIssueId != 0 ) {
        setWindowTitle( tr( "Clone Issue" ) );
        setPrompt( tr( "Clone issue <b>%1</b> as a new issue in folder <b>%2</b>:" ).arg( issue.name(), folder.name() ) );
        setPromptPixmap( IconLoader::pixmap( "issue-clone", 22 ) );
    } else {
        setWindowTitle( tr( "Add Issue" ) );
        setPrompt( tr( "Create a new issue in folder <b>%1</b>:" ).arg( folder.name() ) );
        setPromptPixmap( IconLoader::pixmap( "issue-new", 22 ) );
    }

    if ( !initialize( folder.typeId(), folder.projectId(), true ) )
        return;

    if ( cloneIssueId != 0 )
        setIssueName( issue.name() );

    IssueTypeCache* cache = dataManager->issueTypeCache( folder.typeId() );
    QList<int> attributes = attributeIds();

    AttributeHelper helper;

    for ( int i = 0; i < attributes.count(); i++ ) {
        int attributeId = attributes.at( i );

        DefinitionInfo info = cache->attributeDefinition( attributeId );
        QString value = info.metadata( "default" ).toString();

        value = helper.convertInitialValue( info, value );

        m_values.insert( attributeId, value );

        if ( cloneIssueId != 0 ) {
            ValueEntity entity = ValueEntity::find( cloneIssueId, attributeId );
            setAttributeValue( attributeId, entity.value() );
        } else {
            setAttributeValue( attributeId, value );
        }
    }
}

AddIssueDialog::~AddIssueDialog()
{
}

void AddIssueDialog::accept()
{
    if ( !validate() )
        return;

    QString name = issueName();
    IssueBatch* batch = new IssueBatch( m_folderId, name );

    for ( QMap<int, QString>::const_iterator it = m_values.begin(); it != m_values.end(); ++it ) {
        QString value = attributeValue( it.key() );
        if ( value != it.value() )
            batch->setValue( it.key(), value );
    }

    QString description = descriptionText();
    if ( !description.isEmpty() )
        batch->addDescription( description, descriptionFormat() );

    executeBatch( batch );
}

bool AddIssueDialog::batchSuccessful( AbstractBatch* batch )
{
    m_issueId = ( (IssueBatch*)batch )->issueId();

    return true;
}

EditIssueDialog::EditIssueDialog( int issueId, QWidget* parent ) : IssueDialog( parent ),
    m_issueId( issueId ),
    m_updateFolder( false )
{
    IssueEntity issue = IssueEntity::find( issueId );
    FolderEntity folder = issue.folder();
    m_oldName = issue.name();

    setWindowTitle( tr( "Edit Attributes" ) );
    setPrompt( tr( "Edit attributes of issue <b>%1</b>:" ).arg( m_oldName ) );
    setPromptPixmap( IconLoader::pixmap( "edit-modify", 22 ) );

    if ( !initialize( folder.typeId(), folder.projectId(), false ) )
        return;

    setIssueName( m_oldName );

    foreach ( const ValueEntity& value, issue.values() ) {
        setAttributeValue( value.id(), value.value() );
        m_values.insert( value.id(), value.value() );
    }
}

EditIssueDialog::~EditIssueDialog()
{
}

void EditIssueDialog::setUpdateFolder( bool update )
{
    m_updateFolder = update;
}

void EditIssueDialog::accept()
{
    if ( !validate() )
        return;

    IssueBatch* batch = NULL;

    QString name = issueName();
    if ( name != m_oldName ) {
        batch = new IssueBatch( m_issueId );
        batch->renameIssue( name );
    }

    for ( QMap<int, QString>::const_iterator it = m_values.begin(); it != m_values.end(); ++it ) {
        QString value = attributeValue( it.key() );
        if ( value != it.value() ) {
            if ( !batch )
                batch = new IssueBatch( m_issueId );
            batch->setValue( it.key(), value );
        }
    }

    if ( batch ) {
        batch->setUpdateFolder( m_updateFolder );
        executeBatch( batch );
    } else {
        QDialog::accept();
    }
}

TransferIssueDialog::TransferIssueDialog( QWidget* parent ) : CommandDialog( parent ),
    m_folderCombo( NULL )
{
}

TransferIssueDialog::~TransferIssueDialog()
{
}

void TransferIssueDialog::initialize( int typeId, int folderId, bool requireAdministrator )
{
    QGridLayout* layout = new QGridLayout();

    QLabel* folderLabel = new QLabel( tr( "&Folder:" ), this );
    layout->addWidget( folderLabel, 0, 0 );

    m_folderCombo = new SeparatorComboBox( this );
    layout->addWidget( m_folderCombo, 0, 1 );

    folderLabel->setBuddy( m_folderCombo );

    layout->setColumnStretch( 1, 1 );

    foreach ( const ProjectEntity& project, ProjectEntity::list() ) {
        if ( requireAdministrator && !ProjectEntity::isAdmin( project.id() ) )
            continue;

        bool projectAdded = false;
        foreach ( const FolderEntity& folder, project.folders() ) {
            if ( folder.typeId() != typeId )
                continue;
            if ( !projectAdded ) {
                m_folderCombo->addParentItem( project.name() );
                projectAdded = true;
            }
            m_folderCombo->addChildItem( folder.name(), folder.id() );
            if ( folder.id() == folderId )
                m_folderCombo->setCurrentIndex( m_folderCombo->count() - 1 );
        }
    }

    setContentLayout( layout, true );

    m_folderCombo->setFocus();
}

int TransferIssueDialog::folderId() const
{
    return m_folderCombo->itemData( m_folderCombo->currentIndex() ).toInt();
}

MoveIssueDialog::MoveIssueDialog( int issueId, QWidget* parent ) : TransferIssueDialog( parent ),
    m_issueId( issueId ),
    m_updateFolder( false )
{
    IssueEntity issue = IssueEntity::find( issueId );
    FolderEntity oldFolder = issue.folder();
    m_oldFolderId = oldFolder.id();

    setWindowTitle( tr( "Move Issue" ) );
    setPrompt( tr( "Move issue <b>%1</b> to another folder of the same type:" ).arg( issue.name() ) );
    setPromptPixmap( IconLoader::pixmap( "issue-move", 22 ) );

    bool available = false;

    foreach ( const ProjectEntity& project, ProjectEntity::list() ) {
        if ( !ProjectEntity::isAdmin( project.id() ) )
            continue;

        foreach ( const FolderEntity& folder, project.folders() ) {
            if ( folder.typeId() != oldFolder.typeId() )
                continue;

            if ( folder.id() != m_oldFolderId ) {
                available = true;
                break;
            }
        }
    }

    if ( !available ) {
        showWarning( tr( "There are no available destination folders." ) );
        showCloseButton();
        setContentLayout( NULL, true );
        return;
    }

    initialize( oldFolder.typeId(), m_oldFolderId, true );
}

MoveIssueDialog::~MoveIssueDialog()
{
}

void MoveIssueDialog::setUpdateFolder( bool update )
{
    m_updateFolder = update;
}

void MoveIssueDialog::accept()
{
    if ( !validate() )
        return;

    int newFolderId = folderId();

    if ( newFolderId == m_oldFolderId ) {
        QDialog::accept();
        return;
    }

    IssueBatch* batch = new IssueBatch( m_issueId );
    batch->moveIssue( newFolderId );

    batch->setUpdateFolder( m_updateFolder );

    executeBatch( batch );
}

CloneIssueDialog::CloneIssueDialog( int issueId, QWidget* parent ) : TransferIssueDialog( parent )
{
    IssueEntity issue = IssueEntity::find( issueId );
    FolderEntity oldFolder = issue.folder();

    setWindowTitle( tr( "Clone Issue" ) );
    setPrompt( tr( "Clone issue <b>%1</b> to a folder of the same type:" ).arg( issue.name() ) );
    setPromptPixmap( IconLoader::pixmap( "issue-clone", 22 ) );

    initialize( oldFolder.typeId(), oldFolder.id(), false );
}

CloneIssueDialog::~CloneIssueDialog()
{
}

DeleteIssueDialog::DeleteIssueDialog( int issueId, QWidget* parent ) : CommandDialog( parent ),
    m_issueId( issueId )
{
    IssueEntity issue = IssueEntity::find( issueId );

    setWindowTitle( tr( "Delete Issue" ) );
    setPrompt( tr( "Are you sure you want to delete issue <b>%1</b>?" ).arg( issue.name() ) );
    setPromptPixmap( IconLoader::pixmap( "edit-delete", 22 ) );

    showWarning( tr( "The entire issue history will be permanently deleted." ) );

    setContentLayout( NULL, true );
}

DeleteIssueDialog::~DeleteIssueDialog()
{
}

void DeleteIssueDialog::accept()
{
    IssueBatch* batch = new IssueBatch( m_issueId );
    batch->deleteIssue();

    executeBatch( batch );
}

AddCommentDialog::AddCommentDialog( int issueId, QWidget* parent ) : CommandDialog( parent ),
    m_issueId( issueId )
{
    IssueEntity issue = IssueEntity::find( issueId );

    setWindowTitle( tr( "Add Comment" ) );
    setPrompt( tr( "Add comment to issue <b>%1</b>:" ).arg( issue.name() ) );
    setPromptPixmap( IconLoader::pixmap( "comment", 22 ) );

    QVBoxLayout* layout = new QVBoxLayout();

    m_commentEdit = new MarkupTextEdit( this );
    layout->addWidget( m_commentEdit );

    setContentLayout( layout, false );

    m_commentEdit->setFocus();
}

AddCommentDialog::~AddCommentDialog()
{
}

void AddCommentDialog::setQuote( const QString& title, const QString& text )
{
    QString quote = QString( "[quote %1]\n%2\n[/quote]\n\n" ).arg( title, text );

    m_commentEdit->setInputValue( quote );
    m_commentEdit->setTextFormat( TextWithMarkup );

    m_commentEdit->goToEnd();
}

void AddCommentDialog::accept()
{
    if ( !validate() )
        return;

    IssueBatch* batch = new IssueBatch( m_issueId );
    batch->addComment( m_commentEdit->inputValue(), m_commentEdit->textFormat() );

    executeBatch( batch );
}

EditCommentDialog::EditCommentDialog( int commentId, QWidget* parent ) : CommandDialog( parent ),
    m_commentId( commentId )
{
    ChangeEntity change = ChangeEntity::findComment( commentId );
    m_issueId = change.issueId();
    CommentEntity comment = change.comment();
    m_oldText = comment.text();
    m_oldFormat = comment.format();
    QString identifier = QString( "#%1" ).arg( commentId );

    setWindowTitle( tr( "Edit Comment" ) );
    setPrompt( tr( "Edit comment <b>%1</b>:" ).arg( identifier ) );
    setPromptPixmap( IconLoader::pixmap( "edit-modify", 22 ) );

    QVBoxLayout* layout = new QVBoxLayout();

    m_commentEdit = new MarkupTextEdit( this );
    layout->addWidget( m_commentEdit );

    setContentLayout( layout, false );

    m_commentEdit->setInputValue( m_oldText );
    m_commentEdit->setTextFormat( m_oldFormat );

    m_commentEdit->setFocus();
}

EditCommentDialog::~EditCommentDialog()
{
}

void EditCommentDialog::accept()
{
    if ( !validate() )
        return;

    QString text = m_commentEdit->inputValue();
    TextFormat format = m_commentEdit->textFormat();

    if ( text == m_oldText && format == m_oldFormat ) {
        QDialog::accept();
        return;
    }

    IssueBatch* batch = new IssueBatch( m_issueId );
    batch->editComment( m_commentId, text, format );

    executeBatch( batch );
}

DeleteCommentDialog::DeleteCommentDialog( int commentId, QWidget* parent ) : CommandDialog( parent ),
    m_commentId( commentId )
{
    QString id = QString( "#%1" ).arg( commentId );

    setWindowTitle( tr( "Delete Comment" ) );
    setPrompt( tr( "Are you sure you want to delete comment <b>%1</b>?" ).arg( id ) );
    setPromptPixmap( IconLoader::pixmap( "edit-delete", 22 ) );

    setContentLayout( NULL, true );
}

DeleteCommentDialog::~DeleteCommentDialog()
{
}

void DeleteCommentDialog::accept()
{
    ChangeEntity change = ChangeEntity::findComment( m_commentId );

    IssueBatch* batch = new IssueBatch( change.issueId() );
    batch->deleteComment( m_commentId );

    executeBatch( batch );
}

AddAttachmentDialog::AddAttachmentDialog( int issueId, const QString& path, const QString& url, QWidget* parent ) : CommandDialog( parent ),
    m_issueId( issueId ),
    m_path( path )
{
    QFileInfo info( url );
    QString baseName = info.completeBaseName();
    QString suffix = info.suffix();

    QString fileName;
    if ( suffix.isEmpty() ) {
        baseName.truncate( 40 );
        fileName = baseName.trimmed();
    } else {
        baseName.truncate( 40 - suffix.length() - 1 );
        fileName = baseName.trimmed() + '.' + suffix;
    }

    if ( fileName != info.fileName() ) {
        MessageBox::warning( parent, tr( "Warning" ),
            tr( "The name of the selected file is longer than %1 characters and will be truncated." ).arg( 40 ) );
    }

    IssueEntity issue = IssueEntity::find( issueId );

    setWindowTitle( tr( "Add Attachment" ) );
    setPrompt( tr( "Add an attachment to issue <b>%1</b>:" ).arg( issue.name() ) );
    setPromptPixmap( IconLoader::pixmap( "file-attach", 22 ) );

    QGridLayout* layout = new QGridLayout();

    QLabel* fileLabel = new QLabel( tr( "&File name:" ), this );
    layout->addWidget( fileLabel, 0, 0 );

    m_fileEdit = new InputLineEdit( this );
    m_fileEdit->setMaxLength( 40 );
    m_fileEdit->setFormat( InputLineEdit::FileNameFormat );
    m_fileEdit->setInputValue( fileName );
    layout->addWidget( m_fileEdit, 0, 1 );

    fileLabel->setBuddy( m_fileEdit );

    QLabel* descriptionLabel = new QLabel( tr( "&Description:" ), this );
    layout->addWidget( descriptionLabel, 1, 0 );

    m_descriptionEdit = new InputLineEdit( this );
    m_descriptionEdit->setMaxLength( 80 );
    layout->addWidget( m_descriptionEdit, 1, 1 );

    descriptionLabel->setBuddy( m_descriptionEdit );

    setContentLayout( layout, true );

    QFileInfo fileInfo( path );
    m_size = fileInfo.size();

    Formatter formatter;

    createProgressPanel( m_size, tr( "Size: %1" ).arg( formatter.formatSize( m_size ) ) );

    m_descriptionEdit->setFocus();
}

AddAttachmentDialog::~AddAttachmentDialog()
{
}

void AddAttachmentDialog::accept()
{
    if ( !validate() )
        return;

    QString fileName = m_fileEdit->inputValue();
    QString description = m_descriptionEdit->inputValue();

    IssueBatch* batch = new IssueBatch( m_issueId );
    batch->addAttachment( fileName, description, m_path );

    connect( batch, SIGNAL( uploadProgress( int ) ), this, SLOT( uploadProgress( int ) ) );

    uploadProgress( 0 );

    executeBatch( batch );
}

void AddAttachmentDialog::uploadProgress( int done )
{
    Formatter formatter;

    QString uploaded = formatter.formatSize( done );
    QString total = formatter.formatSize( m_size );

    setProgress( done, tr( "Uploaded: %1 of %2" ).arg( uploaded, total ) );
}

bool AddAttachmentDialog::batchSuccessful( AbstractBatch* batch )
{
    int error = ( (IssueBatch*)batch )->fileError();

    if ( error != QFile::NoError ) {
        showError( tr( "File could not be read." ) );
        return false;
    }

    return true;
}

GetAttachmentDialog::GetAttachmentDialog( int fileId, const QString& path, const QString& url, QWidget* parent ) : CommandDialog( parent ),
    m_fileId( fileId ),
    m_path( path )
{
    ChangeEntity change = ChangeEntity::findFile( fileId );
    FileEntity file = change.file();
    m_size = file.size();

    setWindowTitle( tr( "Download" ) );
    if ( !url.isEmpty() ) {
        setPrompt( tr( "Download attachment <b>%1</b>:" ).arg( file.name() ) );
        setPromptPixmap( IconLoader::pixmap( "file-save-as", 22 ) );
    } else {
        setPrompt( tr( "Open attachment <b>%1</b>:" ).arg( file.name() ) );
        setPromptPixmap( IconLoader::pixmap( "file-open", 22 ) );
    }

    QGridLayout* layout = new QGridLayout();

    QLabel* fileLabel = new QLabel( tr( "Destination:" ), this );
    layout->addWidget( fileLabel, 0, 0 );

    InputLineEdit* fileEdit = new InputLineEdit( this );
    fileEdit->setReadOnly( true );
    if ( url.isEmpty() )
        fileEdit->setInputValue( path );
    else
        fileEdit->setInputValue( url );
    layout->addWidget( fileEdit, 0, 1 );

    QLabel* descriptionLabel = new QLabel( tr( "Description:" ), this );
    layout->addWidget( descriptionLabel, 1, 0 );

    InputLineEdit* descriptionEdit = new InputLineEdit( this );
    descriptionEdit->setReadOnly( true );
    descriptionEdit->setInputValue( file.description() );
    layout->addWidget( descriptionEdit, 1, 1 );

    setContentLayout( layout, true );

    Formatter formatter;

    createProgressPanel( m_size, tr( "Size: %1" ).arg( formatter.formatSize( m_size ) ) );
}

GetAttachmentDialog::~GetAttachmentDialog()
{
}

void GetAttachmentDialog::download()
{
    accept();
}

void GetAttachmentDialog::accept()
{
    ChangeEntity change = ChangeEntity::findFile( m_fileId );

    IssueBatch* batch = new IssueBatch( change.issueId() );
    batch->getAttachment( m_fileId, m_path );

    connect( batch, SIGNAL( downloadProgress( int ) ), this, SLOT( downloadProgress( int ) ) );

    downloadProgress( 0 );

    executeBatch( batch );
}

void GetAttachmentDialog::downloadProgress( int done )
{
    Formatter formatter;

    QString downloaded = formatter.formatSize( done );
    QString total = formatter.formatSize( m_size );

    setProgress( done, tr( "Downloaded: %1 of %2" ).arg( downloaded, total ) );
}

bool GetAttachmentDialog::batchSuccessful( AbstractBatch* batch )
{
    int error = ( (IssueBatch*)batch )->fileError();

    if ( error != QFile::NoError ) {
        showError( tr( "File could not be saved." ) );
        return false;
    }

    return true;
}

EditAttachmentDialog::EditAttachmentDialog( int fileId, QWidget* parent ) : CommandDialog( parent ),
    m_fileId( fileId )
{
    ChangeEntity change = ChangeEntity::findFile( fileId );
    FileEntity file = change.file();
    m_oldName = file.name();
    m_oldDescription = file.description();

    setWindowTitle( tr( "Edit Attachment" ) );
    setPrompt( tr( "Edit attachment <b>%1</b>:" ).arg( m_oldName ) );
    setPromptPixmap( IconLoader::pixmap( "edit-modify", 22 ) );

    QGridLayout* layout = new QGridLayout();

    QLabel* fileLabel = new QLabel( tr( "&File name:" ), this );
    layout->addWidget( fileLabel, 0, 0 );

    m_fileEdit = new InputLineEdit( this );
    m_fileEdit->setMaxLength( 40 );
    m_fileEdit->setFormat( InputLineEdit::FileNameFormat );
    m_fileEdit->setInputValue( m_oldName );
    layout->addWidget( m_fileEdit, 0, 1 );

    fileLabel->setBuddy( m_fileEdit );

    QLabel* descriptionLabel = new QLabel( tr( "&Description:" ), this );
    layout->addWidget( descriptionLabel, 1, 0 );

    m_descriptionEdit = new InputLineEdit( this );
    m_descriptionEdit->setMaxLength( 80 );
    m_descriptionEdit->setInputValue( m_oldDescription );
    layout->addWidget( m_descriptionEdit, 1, 1 );

    descriptionLabel->setBuddy( m_descriptionEdit );

    setContentLayout( layout, true );

    m_fileEdit->setFocus();
}

EditAttachmentDialog::~EditAttachmentDialog()
{
}

void EditAttachmentDialog::accept()
{
    if ( !validate() )
        return;

    QString fileName = m_fileEdit->inputValue();
    QString description = m_descriptionEdit->inputValue();

    if ( fileName == m_oldName && description == m_oldDescription ) {
        QDialog::accept();
        return;
    }

    ChangeEntity change = ChangeEntity::findFile( m_fileId );

    IssueBatch* batch = new IssueBatch( change.issueId() );
    batch->editAttachment( m_fileId, fileName, description );

    executeBatch( batch );
}

DeleteAttachmentDialog::DeleteAttachmentDialog( int fileId, QWidget* parent ) : CommandDialog( parent ),
    m_fileId( fileId )
{
    ChangeEntity change = ChangeEntity::findFile( fileId );
    FileEntity file = change.file();

    setWindowTitle( tr( "Delete Attachment" ) );
    setPrompt( tr( "Are you sure you want to delete attachment <b>%1</b>?" ).arg( file.name() ) );
    setPromptPixmap( IconLoader::pixmap( "edit-delete", 22 ) );

    setContentLayout( NULL, true );
}

DeleteAttachmentDialog::~DeleteAttachmentDialog()
{
}

void DeleteAttachmentDialog::accept()
{
    ChangeEntity change = ChangeEntity::findFile( m_fileId );

    IssueBatch* batch = new IssueBatch( change.issueId() );
    batch->deleteAttachment( m_fileId );

    executeBatch( batch );
}

AddDescriptionDialog::AddDescriptionDialog( int issueId, QWidget* parent ) : CommandDialog( parent ),
    m_issueId( issueId )
{
    IssueEntity issue = IssueEntity::find( issueId );

    setWindowTitle( tr( "Add Description" ) );
    setPrompt( tr( "Add description to issue <b>%1</b>:" ).arg( issue.name() ) );
    setPromptPixmap( IconLoader::pixmap( "description-new", 22 ) );

    QVBoxLayout* layout = new QVBoxLayout();

    m_descriptionEdit = new MarkupTextEdit( this );
    layout->addWidget( m_descriptionEdit );

    setContentLayout( layout, false );

    m_descriptionEdit->setFocus();
}

AddDescriptionDialog::~AddDescriptionDialog()
{
}

void AddDescriptionDialog::accept()
{
    if ( !validate() )
        return;

    IssueBatch* batch = new IssueBatch( m_issueId );
    batch->addDescription( m_descriptionEdit->inputValue(), m_descriptionEdit->textFormat() );

    executeBatch( batch );
}

EditDescriptionDialog::EditDescriptionDialog( int issueId, QWidget* parent ) : CommandDialog( parent ),
    m_issueId( issueId )
{
    IssueEntity issue = IssueEntity::find( issueId );
    DescriptionEntity description = issue.description();
    m_oldText = description.text();
    m_oldFormat = description.format();

    setWindowTitle( tr( "Edit Description" ) );
    setPrompt( tr( "Edit description of issue <b>%1</b>:" ).arg( issue.name() ) );
    setPromptPixmap( IconLoader::pixmap( "edit-modify", 22 ) );

    QVBoxLayout* layout = new QVBoxLayout();

    m_descriptionEdit = new MarkupTextEdit( this );
    layout->addWidget( m_descriptionEdit );

    setContentLayout( layout, false );

    m_descriptionEdit->setInputValue( m_oldText );
    m_descriptionEdit->setTextFormat( m_oldFormat );

    m_descriptionEdit->setFocus();
}

EditDescriptionDialog::~EditDescriptionDialog()
{
}

void EditDescriptionDialog::accept()
{
    if ( !validate() )
        return;

    QString text = m_descriptionEdit->inputValue();
    TextFormat format = m_descriptionEdit->textFormat();

    if ( text == m_oldText && format == m_oldFormat ) {
        QDialog::accept();
        return;
    }

    IssueBatch* batch = new IssueBatch( m_issueId );
    batch->editDescription( text, format );

    executeBatch( batch );
}

DeleteDescriptionDialog::DeleteDescriptionDialog( int issueId, QWidget* parent ) : CommandDialog( parent ),
    m_issueId( issueId )
{
    IssueEntity issue = IssueEntity::find( issueId );

    setWindowTitle( tr( "Delete Description" ) );
    setPrompt( tr( "Are you sure you want to delete description of issue <b>%1</b>?" ).arg( issue.name() ) );
    setPromptPixmap( IconLoader::pixmap( "edit-delete", 22 ) );

    setContentLayout( NULL, true );
}

DeleteDescriptionDialog::~DeleteDescriptionDialog()
{
}

void DeleteDescriptionDialog::accept()
{
    IssueBatch* batch = new IssueBatch( m_issueId );
    batch->deleteDescription();

    executeBatch( batch );
}
