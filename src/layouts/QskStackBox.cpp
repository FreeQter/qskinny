/******************************************************************************
 * QSkinny - Copyright (C) 2016 Uwe Rathmann
 * This file may be used under the terms of the QSkinny License, Version 1.0
 *****************************************************************************/

#include "QskStackBox.h"
#include "QskLayoutConstraint.h"
#include "QskLayoutEngine.h"
#include "QskLayoutItem.h"
#include "QskStackBoxAnimator.h"

#include <qpointer.h>

static qreal qskConstrainedValue( QskLayoutConstraint::Type type,
    const QskControl* control, qreal widthOrHeight )
{
    using namespace QskLayoutConstraint;

    auto constrainFunction =
        ( type == WidthForHeight ) ? widthForHeight : heightForWidth;

    qreal constrainedValue = -1;
    auto stackBox = static_cast< const QskStackBox* >( control );

    for ( int i = 0; i < stackBox->itemCount(); i++ )
    {
        if ( const auto item = stackBox->itemAtIndex( i ) )
        {
            const qreal v = constrainFunction( item, widthOrHeight );
            if ( v > constrainedValue )
                constrainedValue = v;
        }
    }   

    return constrainedValue;
}

class QskStackBox::PrivateData
{
  public:
    PrivateData()
        : currentIndex( -1 )
    {
    }

    int currentIndex;
    QPointer< QskStackBoxAnimator > animator;
};

QskStackBox::QskStackBox( QQuickItem* parent )
    : QskStackBox( false, parent )
{
}

QskStackBox::QskStackBox( bool autoAddChildren, QQuickItem* parent )
    : Inherited( parent )
    , m_data( new PrivateData() )
{
    setAutoAddChildren( autoAddChildren );
}

QskStackBox::~QskStackBox()
{
}

void QskStackBox::setAnimator( QskStackBoxAnimator* animator )
{
    if ( m_data->animator == animator )
        return;

    if ( m_data->animator )
    {
        m_data->animator->stop();
        delete m_data->animator;
    }

    if ( animator )
    {
        animator->stop();
        animator->setParent( this );
    }

    m_data->animator = animator;
}

const QskStackBoxAnimator* QskStackBox::animator() const
{
    return m_data->animator;
}

QskStackBoxAnimator* QskStackBox::animator()
{
    return m_data->animator;
}

QskStackBoxAnimator* QskStackBox::effectiveAnimator()
{
    if ( m_data->animator )
        return m_data->animator;

    // getting an animation from the skin. TODO ...

    return nullptr;
}

QQuickItem* QskStackBox::currentItem() const
{
    return itemAtIndex( m_data->currentIndex );
}

int QskStackBox::currentIndex() const
{
    return m_data->currentIndex;
}

void QskStackBox::layoutItemRemoved( QskLayoutItem*, int index )
{
    if ( index == m_data->currentIndex )
    {
        int newIndex = m_data->currentIndex;
        if ( newIndex == itemCount() )
            newIndex--;

        m_data->currentIndex = -1;

        if ( newIndex >= 0 )
            setCurrentIndex( index );
    }
    else if ( index < m_data->currentIndex )
    {
        m_data->currentIndex--;
        // currentIndexChanged ???
    }

    auto& engine = this->engine();
    if ( engine.itemCount() > 0 && engine.itemAt( 0, 0 ) == nullptr )
    {
        /*
            Using QGridLayoutEngine for a stack layout is actually
            not a good ideas. Until we have a new implementation,
            we need to work around situations, where the layout does
            not work properly with having several items in the 
            same cell. 
            In this particular situation we need to fix, that we lost
            the item from engine.q_grid[0].
            Calling transpose has this side effect.
            
         */
        engine.transpose();
        engine.transpose(); // reverting the call before
    }
}

void QskStackBox::setCurrentIndex( int index )
{
    if ( index < 0 || index >= itemCount() )
    {
        // hide the current item
        index = -1;
    }

    if ( index == m_data->currentIndex )
        return;

    // stop and complete the running transition
    auto animator = effectiveAnimator();
    if ( animator )
        animator->stop();

    if ( window() && isVisible() && isInitiallyPainted() && animator )
    {
        // When being hidden, the geometry is not updated.
        // So we do it now.

        adjustItemAt( index );

        // start the animation
        animator->setStartIndex( m_data->currentIndex );
        animator->setEndIndex( index );
        animator->setWindow( window() );
        animator->start();
    }
    else
    {
        auto item1 = itemAtIndex( m_data->currentIndex );
        auto item2 = itemAtIndex( index );

        if ( item1 )
            item1->setVisible( false );

        if ( item2 )
            item2->setVisible( true );
    }

    m_data->currentIndex = index;
    Q_EMIT currentIndexChanged( m_data->currentIndex );
}

void QskStackBox::setCurrentItem( const QQuickItem* item )
{
    setCurrentIndex( indexOf( item ) );
}

QSizeF QskStackBox::layoutItemsSizeHint() const
{
    qreal width = -1;
    qreal height = -1;

    QSizeF constraint( -1, -1 );
    Qt::Orientations constraintOrientation = 0;

    const auto& engine = this->engine();
    for ( int i = 0; i < engine.itemCount(); i++ )
    {
        const auto layoutItem = engine.layoutItemAt( i );

        if ( layoutItem->hasDynamicConstraint() )
        {
            constraintOrientation |= layoutItem->dynamicConstraintOrientation();
        }
        else
        {
            const QSizeF hint = layoutItem->sizeHint( Qt::PreferredSize, constraint );

            if ( hint.width() >= width )
                width = hint.width();

            if ( hint.height() >= height )
                height = hint.height();
        }
    }

#if 1
    // does this work ???
    if ( constraintOrientation & Qt::Horizontal )
    {
        constraint.setWidth( -1 );
        constraint.setHeight( height );

        for ( int i = 0; i < engine.itemCount(); i++ )
        {
            const auto layoutItem = engine.layoutItemAt( i );

            if ( layoutItem->hasDynamicConstraint() &&
                layoutItem->dynamicConstraintOrientation() == Qt::Horizontal )
            {
                const QSizeF hint = layoutItem->sizeHint( Qt::PreferredSize, constraint );
                if ( hint.width() > width )
                    width = hint.width();
            }
        }
    }

    if ( constraintOrientation & Qt::Vertical )
    {
        constraint.setWidth( width );
        constraint.setHeight( -1 );

        for ( int i = 0; i < engine.itemCount(); i++ )
        {
            const auto layoutItem = engine.layoutItemAt( i );

            if ( layoutItem->hasDynamicConstraint() &&
                layoutItem->dynamicConstraintOrientation() == Qt::Vertical )
            {
                const QSizeF hint = layoutItem->sizeHint( Qt::PreferredSize, constraint );
                if ( hint.height() > height )
                    height = hint.height();
            }
        }
    }
#endif

    return QSizeF( width, height );
}

qreal QskStackBox::heightForWidth( qreal width ) const
{
    return QskLayoutConstraint::constrainedMetric(
        QskLayoutConstraint::HeightForWidth, this, width, qskConstrainedValue );
}

qreal QskStackBox::widthForHeight( qreal height ) const
{
    return QskLayoutConstraint::constrainedMetric(
        QskLayoutConstraint::WidthForHeight, this, height, qskConstrainedValue );
}

void QskStackBox::layoutItemInserted( QskLayoutItem* layoutItem, int index )
{
    Q_UNUSED( index )

    QQuickItem* item = layoutItem->item();
    if ( item == nullptr )
        return;

#if 1
    /*
       In general QGridLayoutEngine supports having multiple entries
       in one cell, but well ...
       So we have to go away from using it and doing the simple use case of
       a stack layout manually. TODO ...

       One problem we ran into is, that a cell is considered being hidden,
       when the first entry is ignored. So for the moment we simply set the
       retainSizeWhenHidden flag, with the cost of having geometry updates
       for invisible updates.
     */
    layoutItem->setRetainSizeWhenHidden( true );
#endif
    if ( itemCount() == 1 )
    {
        m_data->currentIndex = 0;
        item->setVisible( true );

        Q_EMIT currentIndexChanged( m_data->currentIndex );
    }
    else
    {
        item->setVisible( false );
    }
}

#include "moc_QskStackBox.cpp"
