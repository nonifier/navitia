"""Add import_ntfs_to_mimir field in instance

Adding new bool flag to activate the data loading
into mimir, when files are ntfs.

Revision ID: 465a7431358a
Revises: 242138b08c92
Create Date: 2018-02-23 14:50:23.086805

"""

# revision identifiers, used by Alembic.
revision = '465a7431358a'
down_revision = '242138b08c92'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('import_ntfs_in_mimir', sa.Boolean(), nullable=False, server_default='False'))


def downgrade():
    op.drop_column('instance', 'import_ntfs_in_mimir')
